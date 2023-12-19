;****************************************************************************
; TeachZ80 Test Program. 
;
; Original code from John Winans under GNUJ LGPL license
;
; Test the SIO in polled mode. 
; Hexdumps this program to the SIO-A (consoled). Then prints all the printable
; ASCII characters. Then enters an endless loop which polls for an incoming
; character and repeats it.
;****************************************************************************

stacktop:   equ 0   ; end of RAM + 1
            org 0

;##############################################################
; NOTE THAT THE SRAM IS NOT READABLE AT THIS POINT
;##############################################################

    ; Select SRAM low bank 0, idle the SD card
    ld  a,gpio_out_sd_mosi|gpio_out_sd_ssel
    out (gpio_out_0),a

    ;Disable user leds
    ld  a,0
    out (gpio_out_1),a

    ; Copy the FLASH into the SRAM by reading every byte and 
    ; writing it back into the same address.
    ld  hl,0
    ld  de,0
    ld  bc,_end
    ldir

    ; Disable the FLASH and run from SRAM only from this point on.
    in  a,(flash_disable)   ; Dummy-read this port to disable the FLASH.

;##############################################################
; STARTING HERE, WE ARE RUNNING FROM RAM
;##############################################################    
    ld      sp,stacktop

    ; init both SIO channels. Uses /16 divider (115200 from 1.8432 MHZ clk)
    call    sioa_init
    call    siob_init

    ;print hello world string
    ld      hl,helloWorld
    call    puts

    ; dump the RAM copy of the text region 
    ld      hl,0
    ld      bc,_end
    ld      e,1
    call    hexdump

    ;print another message
    ld      hl,alphabet
    call    puts
    call    spew

    ;print another message
    ld      hl,echostring
    call    puts
    call    echo

    ; This should never be reached
halt_loop:
    halt
    jp      halt_loop

;##############################################################
; Print all printable characters once on SIO A
;##############################################################
spew:
    ld      b,0x20      ; ascii space character
spew_loop:
    call    sioa_tx_char
    inc     b
    ld      a,0x7f      ; last graphic character + 1
    cp      b
    jp      nz,spew_loop
    ret

;##############################################################
; Echo characters from SIO back after adding one.
;##############################################################
echo:
    call    sioa_rx_char    ; get a character from the SIO
    ld      b,a
    call    sioa_tx_char    ; print the character
    jp      echo

;##############################################################
; Prints a new line
;##############################################################
crlf:
    ld      b,'\r'
    call    sioa_tx_char    ; print the character
    ld      b,'\n'
    jp      sioa_tx_char    ; print the character
    

;****************************************************************************
; String Messages 
;****************************************************************************
helloWorld: db "\r\n\r\nHello World - This is TeachZ80\r\nThis is the code I currently execute:\r\n\0"
alphabet:   db "\r\nAnd here the printable ASCII characters:\r\n\0"
echostring: db "\r\n\r\nI will now echo whatever you type:\r\n\0"

;****************************************************************************
; Includes 
;****************************************************************************
include '../lib/io.asm'
include '../lib/sio.asm'
include '../lib/hexdump.asm'
include '../lib/puts.asm'

;##############################################################################
; This marks the end of the data that must be copied from FLASH into RAM
;##############################################################################
_end: