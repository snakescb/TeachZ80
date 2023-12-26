;****************************************************************************
;
;    Copyright (C) 2021 John Winans
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
; https://github.com/johnwinans/2063-Z80-cpm
;
;****************************************************************************

;##############################################################
; Write the null-terminated string starting after the call
; instruction invoking this subroutine to the console.
; Clobbers AF, C
;##############################################################
iputs:
        ex      (sp),hl                 ; hl = @ of string to print
	call	.puts_loop
        inc     hl                      ; point past the end of the string
        ex      (sp),hl
        ret

;##############################################################
; Write the null-terminated staring starting at the address in 
; HL to the console.  
; Clobbers: AF, C
;##############################################################
puts:
	push	hl
	call	.puts_loop
	pop	hl
	ret

.puts_loop:
        ld      a,(hl)                  ; get the next byte to send
        or      a
        jr      z,.puts_done            ; if A is zero, return
        ld      c,a
        call    con_tx_char
        inc     hl                      ; point to next byte to write
        jr      .puts_loop
.puts_done:
        ret

;##############################################################
; Print a CRLF 
; Clobbers AF, C
;##############################################################
puts_crlf:
        call    iputs
        defb    '\r\n\0'
        ret

;##############################################################
; Prints the (unsigned) value in HL as a decimal value to the
; console. If e is 1, leading zeros are not printed
; Clobbers nothing
;##############################################################
puts_decimal:
        push	af
	push	de
	push	hl
	push	bc
        ld      bc, -10000
        call    .puts_decimal_start
        ld      bc, -1000
        call    .puts_decimal_start
        ld      bc, -100
        call    .puts_decimal_start
        ld      bc, -10
        call    .puts_decimal_start
        ld      a,'0'
        add     a, l
        ld      c, a
        call    con_tx_char
        pop	bc
	pop	hl
	pop	de
	pop	af
        ret

.puts_decimal_start:
        ld      d,-1                    ; use d as a counter
.puts_decimal_loop:
        inc     d                       ; increment d
        add     hl,bc                   ; hl = hl + bc (bc is negative). if result is < 0, carry is set
        jr      c,.puts_decimal_loop    ; if carry is 1, hl was bigger than -bc, continue loop
        sbc     hl,bc                   ; hl = hl-bc-c. (bc is negative) restores hl to what it was before the last loop
        ld      a, d                    ; check if d is zero
        or      a                       
        jr      nz,.puts_dec_print      ; if not zero, print it
        ld      a, e                    ; if zero, check e
        or      a
        jr      nz,.puts_dec_skip       ; if e is not zero, skip printing d
.puts_dec_print:
        ld      a, d                    ; add ascii of '0'
        add     '0'
        ld      c, a                    ; store in c to print        
        ld      e, 0                    ; clear e, all following characters print now
        call    con_tx_char
.puts_dec_skip:
        ret