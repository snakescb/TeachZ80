;****************************************************************************
;
;   TeachZ80 Bootstrap
;
;   Based on the original code from John Winans, published under GNU LGPL:
;   https://github.com/Z80-Retro/2063-Z80-cpm/blob/main/boot/firmware.asm
;
;   Author:
;   Christian Luethi
;
;   This software is located in the TeachZ80 flash, and is executing the following tasks:
;   - Copy of this code from Flash to RAM address 0000
;   - Disabling the FLASH and continue executing the code from RAM
;   - Initalization of the TeachZ80 Mainboard Hardware (GP outputs, SD Card signals)
;   - Initialization of SIO A, uses /64 divider (115200 from 7.3728 MHZ clk)
;	- Read the MBR from the SD card (first block of 512 bytes)
;	- Access partition 1 on the SD card
;   - Copy 16k of the O/S from partition 1 into RAM at address LOAD_BASE (C000)
;   - Jump to the O/S
;
;	Communication: Boostrap is initializing SIO A (Console) with /64 clock divider.
;	The CTC is not used. Assuming "SIO CLK SELECT" Jumper Channel "A" is in the 
;	"SYN" position, and the TeachZ80 clock channel SIOA is set to 7.3728MHz
;	(default), this will result in a baudrate of 115200 (8N1) on the console output.
;
;	LED Status: Bootstrap uses the red disk led to display the process status:
;	- LED off: After the OS has been loaded successfully, the LED is disabled.
;	  From there, the loaded OS is controlling the SD Card (and therfore the LED)
;	- 1 short blink after reset: This is the welcome blink, indicating bootstrap code started
;	  After the welcome blink, the LED is controlled by the SD read accesses
;	- ERROR CODE 1: No SD-Card in slot
;	- ERROR CODE 2: Unable to initialize SD-Card
;	- ERROR CODE 3: Unable to read MBR on SD-Card
;	- ERROR CODE 4: MBR Invalid or no partition 1 on card
;	- ERROR CODE 5: Error while reading OS from card
;	- ERROR CODE 6: Invalid OS detected
;
;   Version 1.0 - 26.12.2023
;
;****************************************************************************

;****************************************************************************
; Constants
;****************************************************************************
.boot_version_major:	equ	   1
.boot_version_minor:	equ	   0
.low_bank:              equ   14        ; Use low bank 14
.load_blks:	            equ	  32        ; SD blocksize is 512, gives 16k to load

;****************************************************************************
; Required includes (others follow at the bottom of the code) 
;****************************************************************************
include	'../lib/memory.asm'

;##############################################################################
; START FROM FLASH
;############################################################################## 
bootstrap:  org  0x0000						; Z80 cold boot vector

  	ld      a,(gpio_out_cache)				; Select SRAM low bank 14, initialze outputs
    out	    (gpio_out_0),a
	ld		a, 0
	out	    (gpio_out_1),a

	ld	    hl,0							; copy the FLASH into the SRAM
	ld	    de,0							; writing it back into the same address
	ld	    bc,.end
	ldir	

	in	    a,(flash_disable)				; Disable FLASH and run from SRAM

;##############################################################################
; CONTINUE FROM RAM
;############################################################################## 
	ld      sp, LOAD_BASE                  	; Load stackpointer at LOAD_BASE
	call    sioa_init_64                    ; Init the Console SIO, use 64 divider (115200@7.372MHz Clock)
	call    .print_boot_message             ; Display boot message.
		
;******************************************************************************
    call    .boot_sd                        ; Load boot code from the SD card.
;******************************************************************************

    call    iputs                           ; Emergency stop, something went wrong
	db		"\r\nSYSTEM LOAD FAILED! HALTING.\r\n\0"
	call 	.boot_error_blink				; endless error blinking

.halt_loop:
	halt
	jp      .halt_loop    

;##############################################################################
; Load 16K from the first 32 blocks of partition 1 on the SD card into
; memory starting at 'LOAD_BASE' and jump to it.
; If reading the SD card should fail then this function will return.
;
; TODO: Sanity-check the partition type, size and design some sort of 
; signature that can be used to recognize the SD card partition as viable.
;##############################################################################
.boot_sd:

	; ######################### Initialize SD Card #########################
	call 	sd_initialize					; initialize sd card
	or		a								; check if a is zero > card initialized successfully
	jp		z,.load_mbr						; read the first sector

	; ######################### Init Error Printing #########################
	cp		6								; error 6, no card in slot
	jp		nz, .sd_init_print_error		; every other error, print error code
	call	iputs							
	db		"No SD card detected in slot\r\n\0"
	ld		b, 1							; blink error code 1
	ret

.sd_init_print_error:						; print any other error message
	push	af
	call	iputs
	db		"Cannot initialize SD card. Error message: 0x\0"
	pop 	af
	call 	hexdump_a
	call 	puts_crlf
	ld		b, 2							; blink error code 2
	ret

.load_mbr:
	call	iputs
	db		"Booting from SD card\r\nReading Master Boot Record...\0"
	; ########## Read the first block on the card (MBR) to LOAD_BASE ##########
	ld	    hl, 0			                ; SD card block number to read, 32bit
	push    hl			                    ; push high word to stack
	push    hl			                    ; push low word to stack
	ld	    de,LOAD_BASE		            ; where to store the sector data
	call    sd_readBlock					; read the block from the card
	pop	    hl			                    ; remove the block number from the stack again
	pop	    hl								; remove the block number from the stack again
	or	    a                               ; check if a is zero > block read successfully
	jp		z,.check_mbr					; if rading block zero is ok, check the mbr

	; ####################### MBR Read Error Printing #######################
	push	af
	call	iputs
	db		" error\r\nCannot read MBR from SD card. Error message: 0x\0"
	pop 	af
	call 	hexdump_a
	call 	puts_crlf
	ld		b, 3							; blink error code 3
	ret

.check_mbr:
	; ####### Check if MBR has a valid signature and a partition 1 ##########
	ld		ix, LOAD_BASE+0x1FE
	ld		a, (ix+0)						; byte 510 must be 0x55
	cp		0x55							; compare with 0x55
	jp		nz,.check_mbr_invalid_signature	; if not 0x55, error
	ld		a, (ix+1)						; byte 511 must be 0xAA
	cp		0xAA							; compare with 0xAA
	jp		nz,.check_mbr_invalid_signature	; if not 0xAA, error
	ld	    ix,LOAD_BASE+0x01BE+0x08		; check partition 1 start block
	ld	    h,(ix+3)						; block number 31-24 -> H
	ld	    l,(ix+2)						; block number 23-16 -> L
	ld	    d,(ix+1)						; block number 15-08 -> D
	ld	    e,(ix+0)						; block number 07-00 -> E
	ld		a, h							; if AH is not zero, we look ok
	or		l	
	jp		nz,.load_os						; load the card
	ld		a, d							; if DE is not zero, we look ok as well
	or		e	
	jp		nz,.load_os	 					; load the card
	jp		.check_mbr_invalid_startblock	; invalild partition, startblock = 0

	; ##################### Check MBR Error Printing ########################
.check_mbr_invalid_signature:
	call	iputs							
	db		" error\r\nMBR invalidwith invalid signature detected. Please format the card\r\n\0"
	ld		b, 4							; blink error code 4
	ret

.check_mbr_invalid_startblock:
	call	iputs							
	db		" error\r\nMBR without partition 1 detected. Please format the card\r\n\0"
	ld		b, 4							; blink error code 4
	ret

.load_os:
	call	iputs							
	db		" done\r\n\0"
	; ############ Load the OS from the SD card into LOAD_BASE ##############
	call	iputs
	db		'Loading Operating System...\0'

	ld	    ix,LOAD_BASE+0x01BE+0x08		; this will point to the block number of partition 1
	ld	    h,(ix+3)						; block number 31-24 -> H
	ld	    l,(ix+2)						; block number 23-16 -> L
	push    hl								; push high word of the block to read on the stack
	ld	    h,(ix+1)						; block number 15-08 -> H
	ld	    l,(ix+0)						; block number 07-00 -> L
	push	hl								; push low word of the block to read on the stack

	; Stack organization at this point:  	  sp +3 = block number 31-24
											; sp +2 = block number 23-16
											; sp +1 = block number 15-08
											; sp +0 = block number 07-00

	ld	    de,LOAD_BASE					; where to read the sector data into
	ld	    hl,.load_blks					; use hl as a counter, number of blocks to load (should be 32/16KB)

.load_os_loop:
	ld		c,'.'
	call 	con_tx_char						; print dot to show progress
	call 	sd_readBlock					; read the next block from the card
	or		a								; check result
	jp		nz,.boot_error					; if result was not zero, exit with error
	inc		d								; update the destination address
	inc		d								; incrementing d twice, actually will increment de by 512
	ld		ix,0							; increment the 32-bit block number directly on the stack	
	add		ix,sp							; ix = sp
	inc	    (ix+0)							; increment block number 07-00
	jr	    nz,.load_os_loop_next
	inc	    (ix+1)							; if required, increment block number 15-08
	jr	    nz,.load_os_loop_next
	inc	    (ix+2)							; if required, increment block number 23-16
	jr	    nz,.load_os_loop_next
	inc	    (ix+3)							; if required, increment block number 31-24

.load_os_loop_next:
	dec 	hl								; decrement hl
	ld		a,h								; check if it is zero
	or		l
	jr		nz,.load_os_loop				; if not, read next block	

    ; #################### Check Loaded Operating System ####################
.check_os:
	call	iputs
	db		" done\r\n\0"
	ld		a,(LOAD_BASE)					; simply check if the first loaded byte is not 0
	or 		a							
	jr		nz, .boot_os

	; #################### OS Check Error printing ##########################
.check_os_invalid:
	call	iputs							
	db		"Error: Invalid OS detected.\r\n\0"
	pop		hl								; pop low word of the block to read from the stack to hl
	pop		de								; pop high word of the block to read from the stack to de
	ld		b, 6							; blink error code 6
	ret

	; ##################### Run Loaded Operating System #####################
.boot_os:
	call	iputs
	db		'Booting Operating System\r\n\n\0'
	ld		ix, 4							; restore the stack
	add		ix, sp
	ld		sp, ix							; sp = sp + 4
	jp	    z,LOAD_BASE		    			; Run the code that we just read in from the SD card.

.boot_error:
	; ##################### OS Loading Error Printing #######################
	push 	af
	call	iputs
	db		' error\r\n\Could not load O/S from card. Read Error 0x\0'
	pop 	af
	call 	hexdump_a
	call 	puts_crlf
	ld		ix, 4							; restore the stack
	add		ix, sp
	ld		sp, ix							; sp = sp + 4
	ld		b, 5							; blink error code 5
	ret

;****************************************************************************
; Error Blink
; Endless loop blinking the error code given in B
;****************************************************************************
.boot_error_blink:
	call 	.boot_error_blink_pause			; generate pause first when entering this function
.boot_error_blink_loop:
	ld		hl, 0xA0A0						; error code blink on and off time
	call 	.boot_blinkCode					; generate error code blink seuquence
	call 	.boot_error_blink_pause			; generate pause
	jr		.boot_error_blink_loop			; repeat forever
											
.boot_error_blink_pause:					; save the error code
	push 	bc								; push b, which is the error code
	ld		b, 6							; b is the length of the pause 
.boot_error_blink_pause_loop:
	ld		h, 0xFF							; maximum delay time
	call 	.boot_blink_delay				; delay
	djnz	.boot_error_blink_pause_loop	; repeat b times
	pop		bc								; restore the error code
	ret										

;****************************************************************************
; DISK LED Blink function 
;
; B: Message code. 
;	- The LED wil blink b times on-time then off-time
;	- After b repetitions, the function returns
; H: LED on-time
; L: LED off-time
; 
; This function will leave the led OFF (SSEL = high)
;
; Clobbers: A, HL
;****************************************************************************
.boot_blinkCode:
	push 	bc								; save the error code
	ld 		a, (gpio_out_cache)				; disable the LED first
	or		gpio_out_0_sd_ssel				; disk LED is inverted
	ld		(gpio_out_cache), a				; update the cache
	out		(gpio_out_0), a					; set the output
	push	hl								; push hl
.boot_blinkCode_loop:
	ld 		a, (gpio_out_cache)				; enable the LED
	and		~gpio_out_0_sd_ssel				; disk LED is inverted
	out		(gpio_out_0), a					; set the output
	pop		hl								; peek hl. H is on time, L if off time
	push	hl								; peek hl
	call 	.boot_blink_delay				; delay
	ld 		a, (gpio_out_cache)				; disable the LED
	or		gpio_out_0_sd_ssel				; disk LED is inverted
	out		(gpio_out_0), a					; set the output
	pop		hl								; peek hl. H is on time, L if off time
	push	hl								; peek hl
	ld		h, l							; move the "off" time to h
	call 	.boot_blink_delay				; delay
	djnz	.boot_blinkCode_loop			; repeat until b is 0
	pop		hl								; cleanup stack
	pop		bc								; restore the error code
	ret

;****************************************************************************
; Delay function 
; Simple delay for controlliong led blinks
; will delay h amount of units, each unit 0x200 delay loop count lengths
;
; Clobbers A,HL
;****************************************************************************
.boot_blink_delay:							; delay function
	push	bc								; backup b
	ld		b, h							; use b as a counter, h is the amount of "units" to delay
.boot_blink_delay_loop_1:
	ld		hl, 0x200						; 0x200 is the delay "unit"
.boot_blink_delay_loop_2:
	dec		hl								; decrement hl
	ld		a, h							; check if it is zero
	or		l
	jr		nz,.boot_blink_delay_loop_2		; if not, repeat
	djnz	.boot_blink_delay_loop_1		; if yes, repeat the outer loop b times
	pop		bc
	ret										; return

;****************************************************************************
; Prints the welcome message and welcome blink code
;****************************************************************************
.boot_message_divider:
	db		"\r\n\n"
	db		"****************************************************************************\r\n\n\0"
.boot_message:
	db		" TeachZ80 Bootloader\r\n"
	db		" Version: \0"

.print_boot_message:						
	ld  	hl,.boot_message_divider		; print welcome message divider
    call 	puts
    ld  	hl,.boot_message				; print welcome message
    call 	puts
	ld		e, 1
    ld  	hl,.boot_version_major			; print software version major
    call 	puts_decimal
	ld		c,'.'							; print '.'
	call	con_tx_char
    ld  	hl,.boot_version_minor			; print software version minor
    call 	puts_decimal
    ld  	hl,.boot_message_divider		; print welcome message divider
    call 	puts

	ld		hl, 0x40FF						; create welcome blink
	ld		b, 1							; blink 5 times
	call 	.boot_blinkCode					; blink

	ret										; return


;****************************************************************************
; Save area
;****************************************************************************
gpio_out_cache:	 db	 gpio_out_0_sd_mosi|gpio_out_0_sd_ssel|(.low_bank<<4)

;****************************************************************************
; Includes
;****************************************************************************
include	'../lib/io.asm'
include '../lib/sio.asm'
include	'../lib/hexdump.asm'
include '../lib/puts.asm'
include	'../lib/spi.asm'
include	'../lib/sdcard.asm'

;##############################################################################
; This marks the end of the data copied from FLASH into RAM during boot
;##############################################################################
.end:		    equ	$