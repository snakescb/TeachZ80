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

	ld	    hl,0							; copy the FLASH into the SRAM
	ld	    de,0							; writing it back into the same address
	ld	    bc,.end
	ldir	

	in	    a,(flash_disable)				; Disable  FLASH and run from SRAM

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
	db		"\r\nBooting from SD card\r\nReading Master Boot Record ...\0"

	; ######################### Initialize SD Card #########################
	call 	sd_initialize					; initialize sd card
	or		a								; check if a is zero > card initialized successfully
	jp		z,.load_mbr						; read the first sector

	; ######################### Init Error Priting #########################
	cp		6								; error 6, no card in slot
	jp		nz, .sd_init_print_error		; every other error, print error code
	call	iputs							
	db		"\r\nError: No SD card detected in slot\r\n\0"
	ret

.sd_init_print_error:						; print any other error message
	push	af
	call	iputs
	db		"\r\nError: Cannot initialize SD card. Error message: 0x\0"
	pop 	af
	call 	hexdump_a
	call 	puts_crlf
	ret

.load_mbr:
	; ########## Read the first block on the card (MBR) to LOAD_BASE ##########
	ld	    hl, 0			                ; SD card block number to read, 32bit
	push    hl			                    ; push high word to stack
	push    hl			                    ; push low word to stack
	ld	    de,LOAD_BASE		            ; where to store the sector data
	call    sd_readBlock					; read the block from the card
	pop	    hl			                    ; remove the block number from the stack again
	pop	    hl								; remove the block number from the stack again
	or	    a                               ; check if a is zero > block read successfully
	jp		z,.load_os				

	; ####################### MBR Read Error Priting #######################
	push	af
	call	iputs
	db		"\r\nError: Cannot read MBR from SD card. Error message: 0x\0"
	pop 	af
	call 	hexdump_a
	call 	puts_crlf
	ret

.load_os:
	call	iputs							
	db		" done\r\nSD Partition 1 checked OK\r\n\0"

	; ############ Load the OS from the SD card into LOAD_BASE ##############
	call	iputs
	db		'Loading Operating System ...\0'

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
	jr		nz,.boot_error					; if result was not zero, exit with error
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

    ; ##################### Run Loaded Operating System #####################
.boot_os:
	call	iputs
	db		' done\r\nBooting Operating System\r\n\n\0'
	pop		hl								; pop low word of the block to read from the stack to hl
	pop		de								; pop high word of the block to read from the stack to de
	jp	    z,LOAD_BASE		    			; Run the code that we just read in from the SD card.

.boot_error:
	; ################################ ERROR ################################
	push 	af
	call	iputs
	db		'\r\n\nError: Could not load O/S from card. Read Error 0x\0'
	pop 	af
	call 	hexdump_a
	call 	puts_crlf
	call 	puts_crlf
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