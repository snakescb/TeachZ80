;****************************************************************************
; TeachZ80 Test Program. 
;
; Blink the SD card select LED  and the Green LED's in sequence
; Copies FLASH to RAM, then disables the flash
;****************************************************************************

stacktop:   equ 0   ; end of RAM + 1
    org     0x0000   

;##############################################################################
; STARTING HERE, WE ARE RUNNING FROM FLASH
;##############################################################################   
    ld      a, 0                            ; load a with 0
    out     (gpio_out_0), a                 ; turn on disk LED's (it is inverted)
    out     (gpio_out_1), a                 ; turn off gpio LED's and sd led's

    ; Copy the FLASH into the SRAM by reading every byte and 
    ; writing it back into the same address.
    ld  hl,0
    ld  de,0
    ld  bc,_end
    ldir                ; Copy all the code in the FLASH into RAM at same address.

    ; Disable the FLASH and run from SRAM only from this point on.
    in  a,(flash_disable)   ; Dummy-read this port to disable the FLASH.

;##############################################################################
; STARTING HERE, WE ARE RUNNING FROM RAM
;############################################################################## 
    ld      sp,stacktop                 ;initalize stackpointer

main:
    ;turn the Disk LED on
    ld      a, 0                       ; load a with 0
    out     (gpio_out_0), a            ; io write to output 0 

    call delay

    ;turn the Disk LED off
    ld      a, gpio_out_0_sd_ssel       ; load a with 0
    out     (gpio_out_0), a             ; io write to output 0

    ;increment the green leds
    ld      a,(green_leds)              ; read save variable
    inc     a                           ; increment it
    ld      (green_leds),a              ; save it back
    out     (gpio_out_1),a              ; write it to output

    call delay

    ;rePeat
    jp      main

;****************************************************************************
; Waste some time & return 
;****************************************************************************
delay:
    ld      hl,0x8000
dloop:
    dec     hl
    ld      a,h
    or      l
    jp      nz,dloop
    ret


;****************************************************************************
; Save space 
;****************************************************************************
green_leds: db  0

;****************************************************************************
; Includes 
;****************************************************************************
include '../lib/io.asm'

;##############################################################################
; This marks the end of the data that must be copied from FLASH into RAM
;##############################################################################
_end:       equ $
