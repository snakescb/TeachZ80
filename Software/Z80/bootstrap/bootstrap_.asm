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
;   - Initialization of SIO A, uses /16 divider (115200 from 1.8432 MHZ clk)
;	- Read the MBR from the SD card (first block of 512 bytes)
;	- Access partition 1 on the SD card
;   - Copy 16k of the O/S from partition 1 into RAM at address LOAD_BASE (C000)
;   - Jump to the O/S
;
;   Version 1.0 - 21.12.2023
;
;****************************************************************************

;****************************************************************************
; Constants
;****************************************************************************
.boot_version_major:	equ	   1
.boot_version_minor:	equ	   0
.boot_debug:		    equ	   1	    ; Set to 1 to show debug printing, else 0 
.low_bank:              equ   14        ; 2 connected RAM banks at startup
.load_blks:	            equ	  31        ; SD blocksize is 512, gives 16k to load //TBD! Should be 32, but crashes
.stacktop:	            equ LOAD_BASE   ; so the loaded OS will not be overwritten 

;****************************************************************************
; Required includes (others follow at the bottom of the code) 
;****************************************************************************
include	'../lib/memory.asm'

;##############################################################################
; START FROM FLASH
;############################################################################## 
bios:  org 0x0000

  	; Select SRAM low bank 14, initialze outputs
    ld      a,(gpio_out_cache)
    out	    (gpio_out_0),a

    ; Copy the FLASH into the SRAM by reading every byte and 
    ; writing it back into the same address.
	ld	    hl,0
	ld	    de,0
	ld	    bc,.end
	ldir	

    ; Disable the FLASH and run from SRAM only from this point on.
    ; Dummy-read this port to disable the FLASH.
	in	    a,(flash_disable)	

;##############################################################################
; CONTINUE FROM RAM
;############################################################################## 
    ld      sp,.stacktop                    ; Load stackpointer
	call    sioa_init                       ; Init the Console SIO
	call    .print_boot_message             ; Display boot message.
	
;******************************************************************************
    call    .boot_sd                        ; Load bootstrap code from the SD card.
;******************************************************************************

    call    iputs                           ; Emergency stop, something went wrong
	db		'\r\nSYSTEM LOAD FAILED! HALTING.\r\n\0'

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
    call	iputs
	db		'\r\nBooting from SD card partition 1\r\n\nReading SD card MBR...\0'

    in      a,(gpio_in_0)                   ; check if a card is present in the slot
    and     gpio_in_0_sd_det
    jr      z,.boot_sd_0

    call    iputs
	db		'Error: No SD card detected in slot\r\n\0'
	ret

.boot_sd_0:
    call    sd_boot		                    ; transmit 80 CLKs                                        
	call    sd_cmd0                         ; The response byte should be 0x01 (idle) from cmd0
	cp      0x01
	jr	    z,.boot_sd_1

    call    iputs
	db		'Error: Can not read SD card (cmd0 command status not idle)\r\n\0'
	ret

.boot_sd_1:
    ld	    de,LOAD_BASE	                ; Use the load area for a temporary buffer
	call    sd_cmd8		                    ; CMD8 
    ld	    a,(LOAD_BASE)                   ; The response should be: 0x01 0x00 0x00 0x01 0xAA.
	cp	    0x01
	jr	    z,.boot_sd_2

    call	iputs
	db		'Error: Can not read SD card (cmd8 command status not valid):\r\n\0'
	ld	    hl,LOAD_BASE	                ; dump bytes from here
	ld	    e,0		                        ; no fancy formatting
	ld	    bc,5		                    ; dump 5 bytes
	call    hexdump
	call    puts_crlf
    ret

.boot_sd_2:
    .ac41_max_retry:  equ	0x80		    ; limit the number of acmd41 retries
    ld	    b,.ac41_max_retry

.ac41_loop:
	push    bc			                    ; save BC since B contains the retry count 
	ld	    de,LOAD_BASE		            ; store command response into LOAD_BASE
	call    sd_acmd41		                ; ask if the card is ready
	pop	    bc			                    ; restore our retry counter
	or	    a			                    ; check to see if A is zero
	jr	    z,.ac41_done		            ; is A is zero, then the card is ready
	
    ld	    hl,0x1000		                ; count to 0x1000 to consume time, retry
.ac41_dly:
    dec	    hl			                    ; HL = HL -1
	ld	    a,h			                    ; HL == 0?
	or	    l
	jr	    nz,.ac41_dly		            ; if HL != 0 then keep counting
	djnz    .ac41_loop		                ; if (--retries != 0) then try again

    call    iputs                           ; if we enter here, acmd41 failed
	db		'Error: Can not read SD card (acmd41 card not ready)\r\n\0'
	ret

.ac41_done:
	ld	    de,LOAD_BASE                    ; Find out the card capacity (HC or XC)
	call    sd_cmd58                        ; This status is not valid until after ACMD41.
	ld	    a,(LOAD_BASE+1)                 ; Check that CCS=1 here to indicate that we have an HC/XC card
	and	    0x40			 
	jr	    nz,.boot_hcxc_ok

.boot_hcxc_ok:                              ; ############ Read the MBR ############
	ld	    hl,0			                ; SD card block number to read
	push    hl			                    ; high word
	push    hl			                    ; low word
	ld	    de,LOAD_BASE		            ; where to store the sector data
	call    sd_cmd17
	pop	    hl			                    ; remove the block number from the stack again
	pop	    hl
	or	    a                               ; was successful if A is 0
	jr	    z,.boot_cmd17_ok

	call	iputs
	db		'Error: Failed to read MBR on SD Card.\r\n\0'
	ret
	
.boot_cmd17_ok:
    ; ############ Read the first sectors of the first partition ##############
	call	iputs
	db		' done\r\n\0'
	ld	    ix,LOAD_BASE+0x01BE+0x08
    call	iputs
	db		'Partition 1 - Starting block  : \0'
	ld	    a,(ix+3)
	call	hexdump_a
	ld	    a,(ix+2)
	call	hexdump_a
	ld	    a,(ix+1)
	call	hexdump_a
	ld	    a,(ix+0)
	call	hexdump_a
	call	puts_crlf

	call	iputs
	db		'Partition 1 - Number of blocks: \0'
	ld	    a,(ix+7)
	call	hexdump_a
	ld	    a,(ix+6)
	call	hexdump_a
	ld	    a,(ix+5)
	call	hexdump_a
	ld	    a,(ix+4)
	call	hexdump_a
	call	puts_crlf

    ; ############ Read the first sectors of the first partition ############
	ld	    ix,LOAD_BASE+0x01BE+0x08
	ld	    d,(ix+3)
	ld	    e,(ix+2)
	push    de					; push high word of the block to read on the stack
	ld	    d,(ix+1)
	ld	    e,(ix+0)
	push	de					; push low word of the block to read on the stack

	ld	    de,LOAD_BASE		; where to read the sector data into
	ld	    b,.load_blks		; number of blocks to load (should be 32/16KB)


	call	iputs
	db		'\r\nLoading Operating System.\0'
    call	read_blocks
	pop	    de	                ; Remove the 32-bit block number from the stack.
	pop	    de

    ; ##################### Run Loaded Operating System #####################
	or	    a                   ; A=0, blocks successfully copied
	jp	    nz,.boot_error

.boot_os:
	call	iputs
	db		' done\r\n\n\0'
	jp	    z,LOAD_BASE		    ; Run the code that we just read in from the SD card.

.boot_error:
	; ################################ ERROR ################################
	call	iputs
	db		'\r\n\nError: Could not load O/S from partition 1.\r\n\0'
	ret


;############################################################################
;### Read B number of blocks into memory at address DE starting with
;### 32-bit little-endian block number on the stack.
;### Return A=0 = success!
;############################################################################
read_blocks:					
	; start block high			; +12 = starting block number high word
	; start block low           ; +10 = starting block number low word
	; return address            ; +8 = return address
	push	bc			        ; +4 bc
	push	de			        ; +2 de
	push	iy			        ; +0 iy
								; here relative to  the current sp, stack looks like this

	; Move the stack pointer 4 more down the stack, and manually move the number of
	; the next block to read to this area. cmd17 will take this as an argument.
	; de still should point to LOAD_BASE at this point						
	ld	    iy,-4				
	add	    iy,sp			   
	ld	    sp,iy				; sp moved for down the stack, iy = sp

	; copy the first block number to this stack area
	ld	    a,(iy+12)
	ld	    (iy+0),a
	ld	    a,(iy+13)
	ld	    (iy+1),a
	ld	    a,(iy+14)
	ld	    (iy+2),a
	ld	    a,(iy+15)
	ld	    (iy+3),a

.read_block_n:
	ld		c,'.'				 ; print the next dot to see progress
	call	con_tx_char	

	call	sd_cmd17             ; SP is currently pointing at the block number
	or	    a					 ; A=0, block was read successfully
	jr	    nz,.rb_finish		 ; error happened, abort, leave A

	dec	    b                    ; count the block
	jr	    z,.rb_finish		 ; successfully completed!

	inc	    d                    ; d is the high byte of the de registerpair
	inc	    d					 ; increading d twice, increases de (load address), by 512

	; increment the 32-bit block number
	inc	    (iy+0)
	jr	    nz,.read_block_n
	inc	    (iy+1)
	jr	    nz,.read_block_n
	inc	    (iy+2)
	jr	    nz,.read_block_n
	inc	    (iy+3)
	jr	    .read_block_n

.rb_finish:
	ld		iy,+4					; move SP to where is was
	add	    iy,sp			   		; when this function was entered
	ld	    sp,iy						
	pop	    iy
	pop	    de
	pop	    bc
	ret


;****************************************************************************
; Prints the welcome message and generates welcome blink
;****************************************************************************
.boot_message_1:
	db		"\r\n\n****************************************************************************\r\n\n"
	db		" TeachZ80 Bootloader\r\n"
	db		" Version: \0"
.boot_message_2:
    db		'\r\n\n****************************************************************************\r\n\0'

.print_boot_message:
	; print welcome message including version number
    ld  	hl,.boot_message_1
    call 	puts
	ld		e, 1
    ld  	hl,.boot_version_major
    call 	puts_decimal
	ld		c,'.'
	call	con_tx_char
    ld  	hl,.boot_version_minor
    call 	puts_decimal
    ld  	hl,.boot_message_2
    call 	puts

	ld		a, 1
	out		(gpio_out_1), a
	call	.led_delay
	ld		a, 2
	out		(gpio_out_1), a
	call	.led_delay
	ld		a, 4
	out		(gpio_out_1), a
	call	.led_delay
	ld		a, 8
	out		(gpio_out_1), a
	call	.led_delay
	ld		a, 4
	out		(gpio_out_1), a
	call	.led_delay
	ld		a, 2
	out		(gpio_out_1), a
	call	.led_delay
	ld		a, 1
	out		(gpio_out_1), a
	call	.led_delay
	ld		a, 0
	out		(gpio_out_1), a
	ret
.led_delay:
	ld		hl, 0x7000
.led_delay_loop:
	dec 	hl
	ld		a,h
	or		l
	jr		nz,.led_delay_loop
	ret


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