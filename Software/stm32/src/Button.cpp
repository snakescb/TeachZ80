#include <Button.h>

Button::Button(uint8_t pin, uint8_t debounceTime_ms) : _pin(pin), _debounceTime(debounceTime_ms) {
	pinMode(_pin, INPUT);
	_state = digitalRead(_pin);
	_trigger = false;
	_lastLevelChange = millis();
}

void Button::process() {
	bool pinState = digitalRead(_pin);
	if (_state != pinState) {
		_state = pinState;
		if ((millis() - _lastLevelChange >= _debounceTime)  && pinState) _trigger = true;
		_lastLevelChange = millis();
	}
}

bool Button::pushTrigger() {
	bool ret = _trigger;
	_trigger = false;
	return ret;
}