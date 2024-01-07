# IMPORTANT - SET where on your system sd partition 1 is
SD_DEV=/dev/sda1
DISK_DEF=z80-retro-8k-8m
DRIVES_TO_CLEAN=8

#make sure these are rebuilt always 
.PHONY: cpm.bin

default: clean all
all: teachZ80.img

# cleans all assembly outputs and filesystems
clean: 
	rm -f *.lst *.bin *.hex *.sym *.img

# compiles bios.asm which is including cpm-2.2 and creates cpm.bin
cpm.bin: 
	z80asm bios/bios.asm -o cpm.bin --list=cpm.lst --label=cpm.sym -I../lib -Ibios -Icpm-2.2/src

# creates a bootable base image with the cpm including cpm basic programs, assemblers and adventure game
teachZ80.img: cpm.bin
	rm -rf $@
	mkfs.cpm -f ${DISK_DEF} -b $^ $@
	cpmcp -f ${DISK_DEF} $@ cpm-2.2/filesystem/* 0:
	cpmcp -f ${DISK_DEF} $@ filesystem/assemblers/sid/*.com 0:	
	mkfs.cpm -f ${DISK_DEF} disk_b.img
	cpmcp -f ${DISK_DEF} disk_b.img filesystem/adventure/* 0:

# burns image to the start of the defined partition
burn-boot: teachZ80.img
	sudo dd if=$< of=${SD_DEV} bs=512 seek=00x16384 conv=fsync
	sudo dd if=disk_b.img of=${SD_DEV} bs=512 seek=01x16384 conv=fsync

# DANGEROUS: Erases all 16 8mb drives from the sd card and initializes them wit 0xe5
clean-sd: 
	LC_CTYPE=C tr '\0' '\345' < /dev/zero | sudo dd of=$(SD_DEV) bs=8M count=${DRIVES_TO_CLEAN} conv=fsync iflag=fullblock

# creates an all new empty sd card with drive a and b populated
distro: clean teachZ80.img clean-sd burn-boot

# Erases all 16 8mb drives from the sd card and initializes them wit 0xe5
ls: 
	cp filesystem/diskdefs .
	cpmls -f $(DISK_DEF) teachZ80.img 
	rm diskdefs





