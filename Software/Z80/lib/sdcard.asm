;****************************************************************************
;
;   TeachZ80 SD Card Driver
;
;   Based on the original code from John Winans, published under GNU LGPL:
;   https://github.com/Z80-Retro/2063-Z80-cpm/blob/main/lib/sdcard.asm
;
;	An SD card library suitable for talking to SD cards in SPI mode 0.	
;
; 	References:
; 	- SD Simplified Specifications, Physical Layer Simplified Specification, 
;   	Version 8.00:    https://www.sdcard.org/downloads/pls/
;
; 	The details on operating an SD card in SPI mode can be found in 
; 	Section 7 of the SD specification, p242-264.
;
; 	To initialize an SDHC/SDXC card:
; 	- send at least 74 CLKs
; 	- send CMD0 & expect reply message = 0x01 (enter SPI mode)
; 	- send CMD8 (establish that the host uses Version 2.0 SD SPI protocol)
; 	- send ACMD41 (finish bringing the SD card on line)
; 	- send CMD58 to verify the card is SDHC/SDXC mode (512-byte block size)
;
; 	At this point the card is on line and ready to read and write 
; 	memory blocks.
;
; 	- use CMD17 to read one 512-byte block
; 	- use CMD24 to write one 512-byte block
;
;	Author:
;	Christian Luethi
;
;############################################################################

;****************************************************************************
; Constants
;****************************************************************************
.sd_debug: 		equ 1
.sd_cmd0:		db	0|0x40,0,0,0,0,0x94|0x01
.sd_cmd8:		db	8|0x40,0,0,0x01,0xaa,0x86|0x01
.sd_cmd55:		db	55|0x40,0,0,0,0,0x00|0x01
.sd_acmd41:		db	41|0x40,0x40,0,0,0,0x00|0x01
.sd_cmd58:		db	58|0x40,0,0,0,0,0x00|0x01

;############################################################################
; SSEL = HI (deassert)
; wait at least 1 msec after power up
; send at least 74 (80) SCLK rising edges
; Clobbers A, B, C
;############################################################################
.sd_wakeup:
	ld		b,10						; 10*8 = 80 bits to read
	ld		c,0xFF						; set c=0xff, write will leave MOSI high
.sd_wakeup_loop:
	call	spi_write8					; write 8 bits (causes 8 clocks with MOSI high)
	djnz	.sd_wakeup_loop				; if not yet done, do another byte
	ret

;############################################################################
; Set SSEL high or low
; Generate 8 clocks before and after, according SD specification
; Clobbers A, C
;############################################################################
.sd_ssel_low:
	ld		c, 0xFF						; set c=0xff, write will leave MOSI high
	call	spi_write8					; generate 8 clocks
	ld		a,(gpio_out_cache)			; read output cache
	and		~gpio_out_sd_ssel			; clear sd_sel bit
	out		(gpio_out_0),a				; set sd_sel line to the required state
	ld		(gpio_out_cache), a			; update the output cache with the new state
	call	spi_write8					; create another 8 clocks
	ret		

.sd_ssel_high:
	ld		c, 0xFF						; set c=0xff, write will leave MOSI high
	call	spi_write8					; generate 8 clocks
	ld		a,(gpio_out_cache)			; read output cache
	or		gpio_out_sd_ssel			; set sd_sel bit
	out		(gpio_out_0),a				; set sd_sel line to the required state
	ld		(gpio_out_cache), a			; update the output cache with the new state
	call	spi_write8					; create another 8 clocks
	ret		

;############################################################################
; Send the command pointed by IX and reads expected amount of reseponse bytes
; The response is stored in the .sd_scratch buffer. The amount of bytes sent
; in a command is fixed to 6. If e is set to one, the function also controls
; the ssel line. If e is set to one, ssel line needs to be controlled by 
; the calling function (required for read and writes)
;	- IX: Address of buffer to send to card
;	- B : Amount of bytes to read
;	- E : 0 = Do not control SSEL, 1 = control SSEL
;
; When reading, the function tries up to 0x0F times to read a byte with 
; bit 7 cleared according the SD specification. When received, or after, 
; the maximum amount of retries, it continues to read the reamining amount
; of bytes and returns
;
; Clobbers A, IX, IY
;############################################################################
.sd_command:
	push 	de								; for later convenience
	push 	bc								; required by the function
	bit		0,e								; check if bit 0 in e is 0
	jr		z,.sd_command_send				; if so, skip controlling ssel
	call	.sd_ssel_low					; set ssel low
.sd_command_send:
	ld		iy,.sd_scratch					; iy = scratch buffer
	ld		b, 6							; we will send 6 bytes
.sd_command_send_loop:
	ld 		c,(ix+0)						; load next character to send
	call	spi_write8;						; send the character
	inc		ix								; ix point to next character
	djnz	.sd_command_send_loop			; repeat until b is 0							
	ld		b, 0x0f							; b now holds the max amount of read-tries before giving up
.sd_command_read_start:						
	call 	spi_read8						; read one byte
	bit		7,a								; check if bit 7 is set
	jr		z,.sd_command_read_remaining	; if not set, continue reading remaining bytes, else try again
	djnz	.sd_command_read_start			; repeat until b is 0
.sd_command_read_remaining:							
	ld		(iy+0),a						; store the last byte received
	pop		bc								; peek original bc
	push 	bc	
	dec		b								; decrement b
	jr		z,.sd_command_finish			; if here b is zero, we are done
	inc 	iy								; increment destination pointer
.sd_command_read_loop:
	call 	spi_read8						; read next byte
	ld		(iy+0),a						; store it
	inc 	iy	
	djnz	.sd_command_read_loop			; repeat until b is 0
.sd_command_finish:
	pop		bc								; restore 
	pop 	de								; restore 
	bit		0,e								; check if bit 0 in e is 0
	jr		z,.sd_command_exit				; if so, skip controlling ssel
	call	.sd_ssel_high					; set ssel low
.sd_command_exit:
	ret

;############################################################################
; Initialize the SD and make it ready to following block read and writes
;
; Performs the following:
;	- check if a card is installed in the slot
; 	- wake-up the SD card by sending 80 clocks with ssel set high
; 	- send CMD 0, check valid reposnse
;	- send CMD 8, check valid response
;	- loop sending CMD 55 followed by ACMD41, and wait until card becomes ready
;	- send CMD 58 and check card capacity (SD or XC)
;
; Returns the result of the operation in A:
; 	- A = 0 : Card successfully accessed
; 	- A = 1 : CMD 0 unexpected response
; 	- A = 2 : CMD 8 unesxpectd response
; 	- A = 3 : ACMD 41 timed out, card not ready
;	- A = 4 : CMD 58 invalid response ,card not ready
;	- A = 5 : CMD 58 unsupported card format (not SD, XC)
;	- A = 6 : No card in SD slot
;
; Clobbers everything
;############################################################################
sd_initialize:

	; **** Check if there is a card in the slot ******************************	
	in      a,(gpio_in_0)            	; read input port      
    and     gpio_in_0_sd_det			; check if sd_det line is low (card installed)
    jr      z,.sd_access_wakeup			; go to next command
	ld		a, 6						; return error 6
	ret

	; **** Wakeup the card by sending 80 clocks ******************************	
.sd_access_wakeup:
	call	.sd_wakeup					; send 80 clocks

.sd_access_cmd_0:
	; **** SEND CMD 0  - (GO_IDLE) *******************************************	
	; Read one byte, the expected result is 0x01	
	ld		b,1							; 1 byte response expected
	ld 		e,1							; control ssel
	ld 		ix,.sd_cmd0					; send CMD 0
	call 	.sd_command					; send
	ld		a,(.sd_scratch)				; check received byte
	cp		0x01						; must be 0x01
	jr	    z,.sd_access_cmd_8			; jump to next command
	ld		a, 1						; return error 1
	ret									

	; **** SEND CMD 8 (SEND_IF_COND) *****************************************	
	; The response should be: 0x01 0x00 0x00 0x01 0xAA.
	; for simplicity we just check the first byte for now
.sd_access_cmd_8:
	ld		b,5							; 1 byte response expected
	ld 		ix,.sd_cmd8					; send CMD 8
	call 	.sd_command					; send
	ld		a,(.sd_scratch)				; check received byte
	cp		0x01						; must be 0x01
	jr	    z,.sd_access_acmd_41		; jump to next command
	ld		a, 2						; return error 2
	ret	

	; **** SEND ACMD41 (SD_SEND_OP_COND) **************************************	
	; this must be sent after a CMD 55 (APP_CMD), and can take up to one second
	; The response od CMD 55 should be 0x01, ACMD41 should return 0x00 (ready!)
	; if a try does is not successful, waste a bit of time
	ld		d,0xFF						; maximum tries for ACMD55, use d as a counter
.sd_access_acmd_41:
	ld		b,1							; 1 byte response expected for CMD 55
	ld 		ix,.sd_cmd55				; send CMD 55
	call 	.sd_command					; send
	ld		a,(.sd_scratch)				; check received byte
	cp		0x01						; must be 0x01
	jr		nz,.sd_access_acmd_41_delay	; response was not 0x01, delay before next try
	ld 		ix,.sd_acmd41  				; response was 0x01, continue with ACMD41
	call 	.sd_command					; send
	ld		a,(.sd_scratch)				; check received byte
	cp		0x00						; must be 0x00
	jr	    z,.sd_access_cmd_58			; jump to next command if it is
.sd_access_acmd_41_delay:			
	ld 		hl, 0x0100					; loop counter for delay
.sd_access_acmd_41_delay_loop:
	dec		hl							; decrement until hl is 0
	ld		a,h
	or		l
	jr		nz,.sd_access_acmd_41_delay_loop
	dec		d							; decrement d, check if maximum amount of tries exceeded
	jr		nz,.sd_access_acmd_41		; if the counter is not 0, start next try
	ld		a, 3						; else, return error 3
	ret

	; **** SEND CMD 58 (READ_OCR) ********************************************	
	; Check if its SDHC or SDXC card 
	; Only these cards are supported, they have 512 bytes blockside by default		
	; The first response byte expects a 0x00 (card ready)
	; The second response byte expects bit 6 set high
.sd_access_cmd_58:
	ld		b,5							; 5 byte response expected
	ld 		ix,.sd_cmd58				; send CMD 8
	call 	.sd_command					; send
	ld		ix,.sd_scratch				; access response buffer with an index register 
	ld		a,(ix+0)					; check the first recevived byte
	cp		0x00						; check if byte 0 is 0
	jr		z,.sd_access_cmd_58_2		; if so, check send byte received
	ld		a, 4						; else, return error 4
	ret
.sd_access_cmd_58_2:
	ld		ix,.sd_scratch				; access the reponse buffer with an index register
	ld		a,(ix+1)					; check the second recevived byte
	bit		6,a							; check bit 6
	jr		nz,.sd_access_complete		; if it is not zero, everything is ok
	ld 		a, 5						; else, return error 5
	ret
.sd_access_complete:
	ld 		a, 0						; all complete with no error
	ret


;############################################################################
; Read one block of SD data into RAM
;
; Read one block given by the 32-bit (little endian) number at 
; the top of the stack into the buffer given by address in DE.
;
; - clear SSEL line
; - send command
; - read for CMD ACK
; - wait for 'data token'
; - read data block
; - read data CRC
; - set SSEL line
;
; Returns the result of the operation in A:
; 	- A = 0 : Block sucessfully read
; 	- A = 1 : Card not ready
; 	- A = 2 : Timout happened when waiting for data token
;	- A = 3	: Invalid data token received
;
; Clobbers AF, BC, IX, IY
;############################################################################
sd_readBlock:
	; Stack orgqanization at this point:  sp +5 = block number 31-24
										; sp +4 = block number 23-16
										; sp +3 = block number 15-08
										; sp +2 = block number 07-00
										; sp +1 = return @ High
										; sp +0 = return @ Low

	; **** Generate CMD17 command buffer**************************************
	ld		ix,.sd_scratch				; ix = buffer to sd command buffer
	ld		iy,0	
	add		iy,sp						; iy = address if current stackpointer

	ld		(ix+0),17|0x40				; CMD 17 command byte
	ld		a,(iy+5)					; CMD 17 block number 31-24
	ld		(ix+1),a					
	ld		a,(iy+4)					; CMD 17 block number 23-16		
	ld		(ix+2),a
	ld		a,(iy+3)					; CMD 17 block number 15-08
	ld		(ix+3),a
	ld		a,(iy+2)					; CMD 17 block number 07-00
	ld		(ix+4),a
	ld		(ix+5),0x00|0x01			; the CRC byte

	; **** Send command to the card ******************************************
	push	de							; backup de
	call	.sd_ssel_low				; SSEL line low
	ld		b,1							; CMD 17 expects 1 byte reponse
	ld		e,0							; SSEL line controlled manually		
	call	.sd_command					; send command to card
	ld		a,(.sd_scratch)				; check if card returned 0x00 -> ok
	or		a
	jr		z,.sd_read_wait_token		; if zero, start waiting for the token
	ld		b,1							; else, return error 1
	jr		.sd_read_exit		

	; **** Wait for Data token ***********************************************
.sd_read_wait_token:
	ld		bc, 0x1000					; data token loop max tries
.sd_read_wait_token_loop:
	call	spi_read8					; read one byte from the card
	cp		0xFF						; compare with 0xFF
	jr		nz,.sd_check_token 			; if not 0xFF, check the token
	dec		bc							; decrement counter
	ld		a,b
	or		c
	jr		nz,.sd_read_wait_token_loop ; if not zero, next try
	ld		b,2							; if zero, timeout happened, return error 2
	jr		.sd_read_exit

	; **** Check if received to ken is valid *********************************
.sd_check_token:
	cp		0xFE						; 0xFE is the expected token when card is ready to send
	jr		z,.sd_read_data 			; if 0xFE, start reading data
	ld		b,3							; if not, return error 3
	jr		.sd_read_exit	

	; **** Read the data *****************************************************
.sd_read_data:
	pop 	ix							; restore de from stack and store it in ix
	push	ix							; push it to the stack again, it will be removed when functions exits
	ld		bc,512						; use bc as a counter, we read 512 bytes from the card
.sd_read_data_loop:	
	call	spi_read8					; read one byte from the card
	ld		(ix+0),a					; store it
	inc 	ix							; increment destination pointer
	dec		bc							; decrement counter
	ld		a,b	
	or		c
	jr		nz,.sd_read_data_loop		; if counter is not zero, read next byte

	; **** Read CRC **********************************************************
	call	spi_read8					; read first byte and ignore it
	call	spi_read8					; read second byte and ignore it
	ld		b,0							; we are done, return 0

.sd_read_exit:
	call	.sd_ssel_high				; set ssel line again
	pop 	de							; cleanup stack, restire de
	ld		a,b							; copy return code
	ret	

;############################################################################
; A buffer for exchanging messages with the SD card.
;############################################################################
.sd_scratch: 	ds		6