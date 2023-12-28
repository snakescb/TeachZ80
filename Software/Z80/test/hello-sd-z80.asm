;****************************************************************************
;
;    Copyright (C) 2021,2022 John Winans
;
;    This library is free software; you can redistribute it and/or
;    modify it under the terms of the GNU Lesser General Public
;    License as published by the Free Software Foundation; either
;    version 2.1 of the License, or (at your option) any later version.
;
;    This library is distributed in the hope that it will be useful,
;    but WITHOUT ANY WARRANTY; without even the implied warranty of
;    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
;    Lesser General Public License for more details.
;
;    You should have received a copy of the GNU Lesser General Public
;    License along with this library; if not, write to the Free Software
;    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301
;    USA
;
;****************************************************************************

;############################################################################
;
; Test app for the sdcard driver.
;
; Read the data from the SD raw block 0 (where an MBR would normally 
; be present) into RAM starting at address 'LOAD_BASE', dump it out
; and then branch into it to execute it.,
;
; This loader also disable all green user LED's. The flash code later will 
; turn them on, indication that the sd card was read and code was executed
; successfully.
;
;############################################################################

include	'../lib/memory.asm'

	org	0x0000			    ; Cold reset Z80 entry point.

;##############################################################################
; STARTING HERE, WE ARE RUNNING FROM FLASH
;############################################################################## 

	; Disable user leds, select SRAM low bank 0, idle the SD card, and idle printer signals
	ld	a,0
	out	(gpio_out_1),a	
	ld	a,(gpio_out_cache)
	out	(gpio_out_0),a	

	; Copy the FLASH into the SRAM by reading every byte and 
	; writing it back into the same address.
	ld	hl,0
	ld	de,0
	ld	bc,.end
	ldir		; Copy all the code in the FLASH into RAM at same address.

	; Disable the FLASH and run from SRAM only from this point on.
	in	a,(flash_disable)	; Dummy-read this port to disable the FLASH.

;##############################################################################
; STARTING HERE, WE ARE RUNNING FROM RAM
;############################################################################## 
	ld	sp,LOAD_BASE

	; init console SIO. Uses /16 divider (115200 from 1.8432 MHZ clk)
	call	sioa_init
	; Display a hello world message.
	ld	    hl,.boot_msg
	call	puts
	; Load bootstrap code from the SD card.
	call	.test_sd

	call	iputs
	db	'SYSTEM LOAD FAILED! HALTING.\r\n\0'

	; Spin loop here because there is nothing else to do
	; enable red led to show the error
	ld	a,0
	out	(gpio_out_0),a	
.halt_loop:
	halt
	jp	.halt_loop

.boot_msg:
	db	'\r\n\n'
	db	'##############################################################################\r\n'
	db	' TeachZ80 - BOOTSTRAP - Loading 512 bytes SD Code to RAM C000\r\n'
    db	'##############################################################################\r\n'
	db	'\0'

;##############################################################################
; Initialize the SD card and load the first block on the SD card into 
; memory starting at 'LOAD_BASE' and then jump to it.
; If any SD card commands should fail then this function will return.
;##############################################################################
.test_sd:
	call	iputs
	db	'\r\nReading SD card block zero\r\n\n\0'

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
	jp		z,.start_application				

	; ####################### MBR Read Error Priting #######################	
	push	af
	call	iputs
	db		"\r\nError: Cannot read MBR from SD card. Error message: 0x\0"
	pop 	af
	call 	hexdump_a
	call 	puts_crlf
	ret

	; #################### Start the loaded Application ####################
.start_application:
	call	iputs
	db	'Starting code read from SD card now!\r\n\n\0'
	jp	LOAD_BASE			; Go execute what ever came from the SD card

;##############################################################################
; This is a cache of the last written data to the gpio_out port.
; The initial value here is what is written to the latch during startup.
;##############################################################################
gpio_out_cache:	db gpio_out_sd_clk|gpio_out_sd_mosi|gpio_out_sd_ssel

;****************************************************************************
; Includes 
;****************************************************************************
include	'../lib/io.asm'
include	'../lib/sdcard.asm'
include	'../lib/spi.asm'
include	'../lib/hexdump.asm'
include '../lib/sio.asm'
include '../lib/puts.asm'

;##############################################################################
; This marks the end of the data copied from FLASH into RAM during boot
;##############################################################################
.end:	equ	$