;****************************************************************************
; TeachZ80 Test Program. 
;
; Blink the SD card select LED.
; This runs entirely from the FLASH (does not use SRAM).
;****************************************************************************

    org     0x0000   

;############################################################################
; STARTING HERE, WE ARE RUNNING FROM FLASH, NOT USING ANY RAM
;############################################################################   

main:
    ;turn the LED on (disk led is inverted)
    ld      a, gpio_out_0_sd_mosi   
    out     (gpio_out_0), a         
    ld      hl,0x0000           

    ;first delay loop
loop_on:                            
    dec     hl
    ld      a, h
    or      l
    jp      nz, loop_on

    ;turn the LED off (disk led is inverted)
    ld      a,gpio_out_0_sd_mosi | gpio_out_0_sd_ssel
    out     (gpio_out_0), a         
    ld      hl,0x0000

    ;second delay loop
loop_off:                         
    dec     hl
    ld      a,h
    or      l
    jp      nz, loop_off          

    ;rePeat
    jp      main

;****************************************************************************
; Includes 
;****************************************************************************
include '../lib/io.asm'

_end: