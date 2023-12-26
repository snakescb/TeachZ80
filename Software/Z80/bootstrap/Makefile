default: clean all

all: bootstrap

bootstrap: bootstrap.bin

clean:
	rm -f *.lst *.bin

%.bin: %.asm
	z80asm $^ -o $@ --list=$(basename $@).lst



