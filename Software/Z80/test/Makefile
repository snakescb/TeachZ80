default: clean all

all: blink-flash blink-ram

blink-flash: blink-flash.bin
blink-ram: blink-ram.bin
serial-polled: serial-polled.bin

clean:
	rm -f *.lst *.bin

%.bin: %.asm
	z80asm $^ -o $@ --list=$(basename $@).lst



