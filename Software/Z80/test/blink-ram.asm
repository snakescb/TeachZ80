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
    out     (gpio_out_0), a                 ; Select RAM Bank 0, turn off disk LED's (it is inverted)
    ld      a, gpio_out_1_user_0            ; enable user led 0 for starting
    out     (gpio_out_1), a                 ; Turn off User LED's                                            

    ; Copy the FLASH into the SRAM by reading every byte and 
    ; writing it back into the same address.
    ld      hl,0
    ld      de,0
    ld      bc,_end
    ldir                                    ; Copy all the code in the FLASH into RAM at same address.

    ; Disable the FLASH and run from SRAM only from this point on.
    in      a,(flash_disable)               ; Dummy-read this port to disable the FLASH.

;##############################################################################
; STARTING HERE, WE ARE RUNNING FROM RAM
;############################################################################## 
    ld      sp,stacktop                     ;initalize stackpointer

main2: 
    call        delay   
    call        disable_disk                ; disable disk led
    call        delay                       ; delay
    call        enable_disk                 ; enable disk led

    ld          a,(direction)               ; check direction
    and         0x01                           
    jp          z,go_right                  ; if direction is zero, go right, else go left

go_left:
    ld          a,(userleds)                ; load current mask
    rr          a                           ; rotate a to the right
    and         0x0F                        ; if none of the lower 4 bits is set, change direction
    jp          z, go_left_change
    jp          update_leds                 ; jump to update leds

go_left_change:
    ld          a, 0                        ; set direction to 0 (right)
    ld          (direction), a
    ld          a, 2                        ; userledmask to 2
    jp          update_leds                 ; jump to update leds

go_right:
    ld          a,(userleds)                ; load current led state
    rl          a                           ; rotate a to the left
    and         0x0F                        ; if none of the lower 4 bits is set, change direction
    jp          z, go_right_change
    jp          update_leds                 ; jump to update leds
    
go_right_change:
    ld          a, 1                        ; set direction to 1 (left)
    ld          (direction), a
    ld          a, 4                        ; userledmask to 4

update_leds:
    ld          (userleds),a                ; save a in mask variable
    out         (gpio_out_1), a

    jp          main2                       ; repeat

;****************************************************************************
; Delay function - Waste some time & return 
; clobbers hl
;****************************************************************************
delay:
    ld      hl,0x6000
delay_loop:
    dec     hl
    ld      a,h
    or      l
    jp      nz,delay_loop
    ret

;****************************************************************************
; Disables the Disk Led 
; clobbers a
;****************************************************************************
disable_disk:
    ld      a,gpio_out_0_sd_ssel            
    out     (gpio_out_0),a
    ret

;****************************************************************************
; Enables the Disk Led 
; clobbers a
;****************************************************************************
enable_disk:
    ld      a,0            
    out     (gpio_out_0),a 

    ; test for stm32 ioreq
    inc     b
    ld      a,(stm32_data)                  ; Load stm32_data to a
    inc     a                               ; increment it
    ld      (stm32_data),a                  ; store back 
    out     (stm32_port),a                  ; send it to stm32
    ret

;****************************************************************************
; Save space
;****************************************************************************
direction:      db  1
userleds:       db  1
stm32_data:     db  0

;****************************************************************************
; Includes 
;****************************************************************************
include '../lib/io.asm'

;##############################################################################
; This marks the end of the data that must be copied from FLASH into RAM
;##############################################################################
_end:       equ $
