#include <Led.h>

Led::Led(uint8_t pin) : _pin(pin), _state(0) {
	pinMode(_pin, OUTPUT);
	digitalWrite(_pin, false);
}

void Led::set(uint16_t on, uint16_t off) {
	if (on == 0) {
		digitalWrite(_pin, false);
		_state = 0;
	}
	else if (off == 0) {
		digitalWrite(_pin, true);
		_state = 0;
	}
	else {
		digitalWrite(_pin, true);
		_off = off; _on = on; _state = 1;		
		_event = millis();
	}
}

void Led::process() {	
	if ((_state == 1) && (millis() - _event >= _on)) {
		digitalWrite(_pin, false);
		_state = 2;
		_event = millis();
	}
	if ((_state == 2) && (millis() - _event >= _off)) {
		digitalWrite(_pin, true);
		_state = 1;
		_event = millis();
	}
}
