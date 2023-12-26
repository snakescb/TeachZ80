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

;****************************************************************************
; This program must be loaded to the SD card to the raw storage
; This will make everything else on the SD card unusable!!
;
; ASSUMING your card is /dev/sda (normally the case on raspberry)
; sudo dd if=/dev/zero of=/dev/sda bs=512 count=1
; sudo dd if=hello-sd-card.bin of=/dev/sda bs=512
;****************************************************************************

include	'../lib/memory.asm'
	org	LOAD_BASE		; the second-stage load address
	ld	sp,LOAD_BASE	; everything below LOAD BASE is available to the stack

	; XXX Note that the boot loader should have initialized the SIO, CTC etc.
	; XXX Therefore we can just write to them from here.
	ld	    hl,.boot_msg
	call	puts

.loop:
	; turn on user led's
	ld 	a, gpio_out_1_user_0|gpio_out_1_user_1|gpio_out_1_user_2|gpio_out_1_user_3
	out (gpio_out_1), a
	ld	 b, 20
	call delay

	; turn off user led's
	ld 	a, 0
	out (gpio_out_1), a
	ld	 b, 5
	call delay

	jp .loop

;****************************************************************************
; Delay function - Waste time & return - repeats the loop b times
; clobbers hl, b
;****************************************************************************
delay:
    ld      hl,0x2000
delay_loop:
    dec     hl
    ld      a,h
    or      l
    jr      nz,delay_loop
	dec 	b
	jr		nz,delay
    ret

;****************************************************************************
; Constants 
;****************************************************************************

.boot_msg:
	db	"******************************************************************************\r\n"
	db	" SUCCESS! - This code was loaded from the SD-Card!\r\n"
	db	" I blink some green LED's now to celebrate\r\n"
    db	"******************************************************************************\r\n\0"

;****************************************************************************
; Includes 
;****************************************************************************
include	'../lib/io.asm'
include	'../lib/hexdump.asm'
include '../lib/sio.asm'
include '../lib/puts.asm'