;****************************************************************************
; Configure the BIOS drive DPH structures here.
;
; WARNING
; 	Do *NOT* expected to mount the same drive more than one 
;	way and expect it to work without corrupting the drive!
;****************************************************************************
; List of available disk drivers
disk_driver_nochache:	equ 	1
disk_driver_dmcache:	equ 	2

; Driver to use in bios
cpm_disk_driver:	equ		disk_driver_nochache


if cpm_disk_driver = disk_driver_nochache

	; The SD card offset specified here is relative to the start of the
	; boot partition. The 32-bit boot partition base address is
	; expected to be stored in disk_offset_hi and disk_offset_low
	include 'disk_nocache.asm'
	.dph0:	nocache_dph    0x0000 0x0000	; SD logical drive 0  A:
	.dph1:	nocache_dph    0x0000 0x4000	; SD logical drive 1  B:
	.dph2:	nocache_dph    0x0000 0x8000	; SD logical drive 2  C:
	.dph3:	nocache_dph    0x0000 0xc000	; SD logical drive 3  D:
	.dph4:	nocache_dph    0x0001 0x0000	; SD logical drive 4  E:
	.dph5:	nocache_dph    0x0001 0x4000	; SD logical drive 5  F:
	.dph6:	nocache_dph    0x0001 0x8000	; SD logical drive 6  G:
	.dph7:	nocache_dph    0x0001 0xc000	; SD logical drive 7  H:

	dph_vec:
		dw	.dph0
		dw	.dph1
		dw	.dph2
		dw	.dph3
		dw	.dph4
		dw	.dph5
		dw	.dph6
		dw	.dph7
endif


if cpm_disk_driver = disk_driver_dmcache

	; NOTE: dmcache ONLY works on a single-partition starting at SD block number 0x0800
	include 'disk_dmcache.asm'
	.dph0:	dmcache_dph	0x0000 0x0800	; This is absolute, NOT partition-relative!

	dph_vec:
		dw	.dph0

endif

dph_vec_num:	equ	($-dph_vec)/2		; number of configured drives
