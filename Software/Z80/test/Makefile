default: clean all

all: blink-flash blink-ram serial-polled

blink-flash: blink-flash.bin
blink-ram: blink-ram.bin
serial-polled: serial-polled.bin
hello-sd-card: hello-sd-card.bin
hello-sd-z80: hello-sd-z80.bin

clean:
	rm -f *.lst *.bin

%.bin: %.asm
	z80asm $^ -o $@ --list=$(basename $@).lst



