/* -------------------------------------------------------------------------------------------------------
 teachZ80 Support processor firmware

 Author   : Christian Luethi
--------------------------------------------------------------------------------------------------------- */
#include <Arduino.h>
#include <Led.h>
#include <Button.h>
#include <Si5153.h>
#include <Z80bus.h>
#include <FlashLoader.h>

/* Types and definitions -------------------------------------------------------------------------------- */
enum ApplicationState : byte { Idle, Test };

/* Variables and instances ------------------------------------------------------------------------------ */
ApplicationState applicationState = Idle;
Led statusLed(PA4);
Button button(PH3);
Si5153 clock(PA3, PA5);
Z80bus z80bus;
FlashLoader flashloader(z80bus);

/*#########################################################################################################
 Main Program
##########################################################################################################*/

/* Initialization --------------------------------------------------------------------------------------- */
void setup() {
	Serial.begin(115200);
	statusLed.on();
	if (clock.begin()) {
		//100khz clock
		clock.configureChannel(0, 12800000, 128);
		//150khz clock
		clock.configureChannel(1, 19200000, 128, 1);
		//1MHz clock
		clock.configureChannel(2, 64000000, 64, 1, true);
		Serial.println("Clockgenerator OK");
	}
	else Serial.println("Clockgenerator ERROR");
}

/* Main loop -------------------------------------------------------------------------------------------- */
void loop() {
   
   	z80bus.process();
	flashloader.process();
   	statusLed.process();
   	button.process();

	switch (applicationState) {
		case Idle: {
			if (button.pushTrigger()) {
				statusLed.set(300, 700);
				applicationState = Test;
			}
			break;
   		}

    	case Test: {
			if (button.pushTrigger()) {
				statusLed.set(10, 990);
				applicationState = Idle;
			}
			break;
   		}
    
   		default: break;
   	}

}
