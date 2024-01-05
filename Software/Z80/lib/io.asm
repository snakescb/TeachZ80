
;****************************************************************************
;   TeachZ80 I/O definitions. 
;
;   Original code from John Winans, Z80 Retro!
;****************************************************************************

;****************************************************************************
;  TeachZ80 Version 1 IO port definitions
;****************************************************************************
gpio_in_0:          equ     0x00        ; GP input port 0
gpio_in_1:          equ     0x50        ; GP input port 1
gpio_out_0:         equ     0x10        ; GP output port 0
gpio_out_1:         equ     0x20        ; GP output port 1, only lower 4 bits available
stm32_port:         equ     0x60        ; for communicaation with stm32

sio_ad:             equ     0x30        ; SIO port A, data
sio_bd:             equ     0x31        ; SIO port B, data
sio_ac:             equ     0x32        ; SIO port A, control
sio_bc:             equ     0x33        ; SIO port B, control

ctc_0:              equ     0x40        ; CTC port 0
ctc_1:              equ     0x41        ; CTC port 1
ctc_2:              equ     0x42        ; CTC port 2
ctc_3:              equ     0x43        ; CTC port 3

flash_disable:      equ     0x70        ; dummy-read from this port to disable the FLASH and switch to RAM

;****************************************************************************
;  TeachZ80 Bit Assignments
;****************************************************************************
; GP-Output-0 ---------------------------------------------------------------
gpio_out_0_sd_mosi: equ     0x01
gpio_out_0_sd_clk:  equ     0x02
gpio_out_0_sd_ssel: equ     0x04
gpio_out_0_user_8:  equ     0x08
gpio_out_0_a15:     equ     0x10
gpio_out_0_a16:     equ     0x20
gpio_out_0_a17:     equ     0x40
gpio_out_0_a18:     equ     0x80

; GP-Output-1 ---------------------------------------------------------------
gpio_out_1_user_0:  equ     0x01
gpio_out_1_user_1:  equ     0x02
gpio_out_1_user_2:  equ     0x04
gpio_out_1_user_3:  equ     0x08
gpio_out_1_user_4:  equ     0x10
gpio_out_1_user_5:  equ     0x20
gpio_out_1_user_6:  equ     0x40
gpio_out_1_user_7:  equ     0x80

; GP-Input-0 ----------------------------------------------------------------
gpio_in_0_user_0:   equ     0x01
gpio_in_0_user_1:   equ     0x02
gpio_in_0_user_2:   equ     0x04
gpio_in_0_user_3:   equ     0x08
gpio_in_0_user_4:   equ     0x10
gpio_in_0_user_5:   equ     0x20
gpio_in_0_sd_det:   equ     0x40
gpio_in_0_sd_miso:  equ     0x80

; GP-Input-1 ----------------------------------------------------------------
gpio_in_1_user_6:   equ     0x01
gpio_in_1_user_7:   equ     0x02
gpio_in_1_user_8:   equ     0x04
gpio_in_1_user_9:   equ     0x08


;****************************************************************************
;  Z80 Retro! definitions kept to maintain compatibility with Johns Software
;****************************************************************************
; Z80 Retro Rev 3 IO port definitions
gpio_in:            equ     0x00        ; GP input port
gpio_out:           equ     0x10        ; GP output port
prn_dat:            equ     0x20        ; printer data out

; bit-assignments for General Purpose output port 
gpio_out_sd_mosi:   equ     0x01
gpio_out_sd_clk:    equ     0x02
gpio_out_sd_ssel:   equ     0x04
gpio_out_prn_stb:   equ     0x08
gpio_out_a15:       equ     0x10
gpio_out_a16:       equ     0x20
gpio_out_a17:       equ     0x40
gpio_out_a18:       equ     0x80

; bit-assignments for General Purpose input port 
gpio_in_prn_err:    equ     0x01
gpio_in_prn_stat:   equ     0x02
gpio_in_prn_papr:   equ     0x04
gpio_in_prn_bsy:    equ     0x08
gpio_in_prn_ack:    equ     0x10
gpio_in_user1:      equ     0x20 
gpio_in_sd_det:     equ     0x40
gpio_in_sd_miso:    equ     0x80

; a bitmask representing all of the lobank address bits 
gpio_out_lobank:	equ	0|(gpio_out_a15|gpio_out_a16|gpio_out_a17|gpio_out_a18)