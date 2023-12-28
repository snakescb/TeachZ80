;****************************************************************************
;
;   TeachZ80 SPI Driver
;
;   Based on the original code from John Winans, published under GNU LGPL:
;   https://github.com/Z80-Retro/2063-Z80-cpm/blob/main/lib/spi.asm
;
;	An SPI library suitable for tallking to SD cards.	
;
;   Author:
;   Christian Luethi
;
;****************************************************************************

;############################################################################
;
; This library implements SPI mode 0 (SD cards operate on SPI mode 0.)
; Data changes on falling CLK edge & sampled on rising CLK edge:
;        __                                             ___
; /SSEL    \______________________ ... ________________/      Host --> Device
;                 __    __    __   ... _    __    __
; CLK    ________/  \__/  \__/  \__     \__/  \__/  \______   Host --> Device
;        _____ _____ _____ _____ _     _ _____ _____ ______
; MOSI        \_____X_____X_____X_ ... _X_____X_____/         Host --> Device
;        _____ _____ _____ _____ _     _ _____ _____ ______
; MISO        \_____X_____X_____X_ ... _X_____X_____/         Host <-- Device
;
;############################################################################

;############################################################################
; Write 8 bits in C to the SPI port and discard the received data.
; It is assumed that the gpio_out_cache value matches the current state
; It also assumes, clock is 0 and mosi is 1 when entering
; This will leave: CLK=0, MOSI=1
; Clobbers: A
;############################################################################
spi_write1:	macro bitpos
	and		~gpio_out_sd_clk	; clear clock
	and 	~gpio_out_sd_mosi	; clear mosi
	bit		bitpos,c			; is the bit of C a 1?
	jr		z,.write1_low			
	or		gpio_out_sd_mosi	; set mosi again
.write1_low: ds 0				; for some reason, these labels disappear if there is no neumonic
	out		(gpio_out),a		; clock falling edge + set data
	or		gpio_out_sd_clk		; clock rising edge
	out		(gpio_out),a		
	endm

spi_write8:
	ld		a,(gpio_out_cache)	; get current gpio_out value
	spi_write1	7				; send bit 7
	spi_write1	6				; send bit 6
	spi_write1	5				; send bit 5
	spi_write1	4				; send bit 4
	spi_write1	3				; send bit 3
	spi_write1	2				; send bit 2
	spi_write1	1				; send bit 1
	spi_write1	0				; send bit 0
	and		~gpio_out_sd_clk	; clear clock
	or		gpio_out_sd_mosi	; set mosi
	out		(gpio_out),a
	ret

;############################################################################
; Read 8 bits from the SPI & return it in A.
; This will leave: CLK=0, MOSI=Unchanged (should be one)
; Clobbers A, E
;############################################################################
spi_read1:	macro
	or		gpio_out_sd_clk		; set clock
	out		(gpio_out),a		; clock rising edge
	push 	af					; push a, as we change it after
	in		a,(gpio_in)			; read MISO
	and 	gpio_in_sd_miso		; 
	or		e					; accumulate the current MISO value
	ld		e,a					; NOTE: note this only works because gpio_in_sd_miso = 0x80
	rlc		e					; rotate the data buffer
	pop		af					; load the output chache again
	and		~gpio_out_sd_clk	; clear the clock bit
	out		(gpio_out),a		; clock falling edge edge
	endm

spi_read8:
	ld		e,0					; prepare to accumulate the bits into E
	ld		a,(gpio_out_cache)	; get current gpio_out value
	spi_read1					; read bit 7
	spi_read1					; read bit 6
	spi_read1					; read bit 5
	spi_read1					; read bit 4
	spi_read1					; read bit 3
	spi_read1					; read bit 2
	spi_read1					; read bit 1
	spi_read1					; read bit 0
	ld a,e						; final value in a
	ret