;****************************************************************************
; TeachZ80 Test Program. 
;
; Blink the Disk activity and User IO LED's.
; This runs entirely from the FLASH (does not use SRAM).
;****************************************************************************
    org     0x0000   

;############################################################################
; STARTING HERE
;############################################################################   
    ld      hl,0x0000                       ; initialize hl with 0, will be used for delays later

main:
    ld      a, 0                            ; load a, so it enables the disk led
    out     (gpio_out_0), a                 ; turn the disk led on
    ld      a, gpio_out_1_user_0            ; load a, so it enable user led 0
    out     (gpio_out_1), a                 ; turn user led 0 on

delay_1:                            
    dec     hl                  
    ld      a, h
    or      l
    jp      nz, delay_1

    ld      a, gpio_out_0_sd_ssel           ; load a, so it disables the disk led
    out     (gpio_out_0), a                 ; turn the disk led on  

delay_2:                            
    dec     hl                  
    ld      a, h
    or      l
    jp      nz, delay_2

    ld      a, 0                            ; load a, so it enables the disk led
    out     (gpio_out_0), a                 ; turn the disk led on
    ld      a, gpio_out_1_user_1            ; load a, so it enable user led 1
    out     (gpio_out_1), a                 ; turn user led 0 on

delay_3:                            
    dec     hl                  
    ld      a, h
    or      l
    jp      nz, delay_3

    ld      a, gpio_out_0_sd_ssel           ; load a, so it disables the disk led
    out     (gpio_out_0), a                 ; turn the disk led on    

delay_4:                            
    dec     hl                  
    ld      a, h
    or      l
    jp      nz, delay_4

    ld      a, 0                            ; load a, so it enables the disk led
    out     (gpio_out_0), a                 ; turn the disk led on
    ld      a, gpio_out_1_user_2            ; load a, so it enable user led 2
    out     (gpio_out_1), a                 ; turn user led 0 on       

delay_5:                            
    dec     hl                  
    ld      a, h
    or      l
    jp      nz, delay_5

    ld      a, gpio_out_0_sd_ssel           ; load a, so it disables the disk led
    out     (gpio_out_0), a                 ; turn the disk led on    

delay_6:                            
    dec     hl                  
    ld      a, h
    or      l
    jp      nz, delay_6

    ld      a, 0                            ; load a, so it enables the disk led
    out     (gpio_out_0), a                 ; turn the disk led on
    ld      a, gpio_out_1_user_3            ; load a, so it enable user led 3
    out     (gpio_out_1), a                 ; turn user led 0 on

delay_7:                            
    dec     hl                  
    ld      a, h
    or      l
    jp      nz, delay_7

    ld      a, gpio_out_0_sd_ssel           ; load a, so it disables the disk led
    out     (gpio_out_0), a                 ; turn the disk led on  

delay_8:                            
    dec     hl                  
    ld      a, h
    or      l
    jp      nz, delay_8

    ;repeat
    jp      main

;****************************************************************************
; Waste some time & return 
;****************************************************************************
delay:
    ld      hl,0x0000
delay_loop:
    dec     hl
    ld      a,h
    or      l
    jp      nz,delay_loop
    ret

;****************************************************************************
; Includes 
;****************************************************************************
include '../lib/io.asm'

_end: