defaultTarget: clean all

all: blink-flash.bin

clean:
	rm -f *.lst *.bin

%.bin: %.asm
	z80asm $^ -o $@ --list=$(basename $@).lst



