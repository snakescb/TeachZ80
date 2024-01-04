default: clean all

all: boot

boot: boot.bin

clean:
	rm -f *.lst *.bin

%.bin: %.asm
	z80asm $^ -o $@ --list=$(basename $@).lst



