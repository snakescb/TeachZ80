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
;############################################################################

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
	ldir

	; Disable the FLASH and run from SRAM only from this point on.
	in	a,(flash_disable)	; Dummy-read this port to disable the FLASH.

;##############################################################################
; STARTING HERE, WE ARE RUNNING FROM RAM
;############################################################################## 
	ld		sp, 0
	call    sioa_init_64                    ; Init the Console SIO, use 64 divider (115200@7.372MHz Clock)
	call    .print_boot_message             ; Display boot message.

	call	sd_initialize					; initialize card

	ld	    hl, 0			                ; SD card block number to read, 32bit
	push    hl			                    ; push high word to stack
	push    hl			                    ; push low word to stack
	ld	    de, sd_scratch		            ; where to store and read the sector data from

	call    sd_readBlock					; read the block from the card
	pop	    hl			                    ; pop block number low byte 
	inc		hl								; increment it
	push 	hl								; push it back
	call    sd_readBlock					; read another block
	pop	    hl			                    ; pop block number low byte 
	inc		hl								; increment it
	push 	hl								; push it back
	call    sd_writeBlock					; write block
	pop	    hl			                    ; pop block number low byte 
	inc		hl								; increment it
	push 	hl								; push it back
	call    sd_writeBlock					; write another block

	pop		hl								; cleanup stack
	pop	    hl							
					
.halt_loop:
	halt
	jp	.halt_loop

;****************************************************************************
; Prints the welcome message and generates welcome blink
;****************************************************************************
.boot_message:
	db		"\r\n\n"
	db		"****************************************************************************\r\n"
	db		" TeachZ80 Simple SD Test\r\n"
    db		"****************************************************************************\r\n\0"

.print_boot_message:
    ld  	hl,.boot_message
    call 	puts
	ret

;****************************************************************************
; Save area
;****************************************************************************
gpio_out_cache:	 db	 gpio_out_0_sd_mosi|gpio_out_0_sd_ssel
sd_scratch:		 ds	 512

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