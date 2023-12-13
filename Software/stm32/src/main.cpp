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
#define FLASH_MODE_DEACTIVATION_TIME_S  10

enum ApplicationState : byte { Idle, FlashMode };

/* Variables and instances ------------------------------------------------------------------------------ */
ApplicationState applicationState = Idle;
Led statusLed(PA4);
Button button(PH3);
Si5153 clock(PA3, PA5);
Z80bus z80bus;
FlashLoader flashloader(z80bus);

uint32_t appTiming;

/*#########################################################################################################
 Main Program
##########################################################################################################*/

/* Initialization --------------------------------------------------------------------------------------- */
void setup() {
	Serial.setTx(PA9);
	Serial.setRx(PA10);	
	Serial.begin(115200);
	statusLed.on();

	if (clock.begin()) {	
		clock.configureChannel(0, 10000000, 1, 0); //10.000 MHz System Cock
		clock.configureChannel(1, 1843200, 1, 1);  //1.8432 MHZ Clock SIOA
		clock.configureChannel(2, 1843200, 1, 1);  //1.8432 MHZ Clock SIOA
	}

	z80bus.resetZ80();
	delay(500);
	statusLed.set(1, 3998);
}

/* Main loop -------------------------------------------------------------------------------------------- */
void loop() {
   
	//Process modules
   	z80bus.process();
	flashloader.process();
   	statusLed.process();
   	button.process();

	//Reception of serial characters
	int rx = Serial.read();
	if (rx != -1) {
		if (flashloader.flashmode == flashloader.active) flashloader.serialUpdate((uint8_t) rx);			
	}

	//state machine
	switch (applicationState) {
		case Idle: {
			if (button.pushTrigger()) {				
				Serial.println("Entering Flash Mode");
				flashloader.setFlashMode(true);				
				statusLed.set(200, 200);
				applicationState = FlashMode;	
				appTiming = millis();			
			}
			break;
   		}

    	case FlashMode: {
			
			if (button.pushTrigger()) {
				Serial.println("Flash Mode Aborted");
				flashloader.setFlashMode(false);	
				statusLed.set(1, 3998);
				applicationState = Idle;
			}
			else if (flashloader.flashmode == flashloader.inactive) {
				Serial.println("Flash Mode completed");
				statusLed.set(1, 3998);
				applicationState = Idle;
			}
			break;
   		}
    
   		default: break;
   	}

}

/* -------------------------------------------------------------------------------------------------------
 STM32CubeMX Generated Clock Initialization, 80MHz Systemclock
--------------------------------------------------------------------------------------------------------- */
void SystemClock_Config(void) {

  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK) Error_Handler();

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSEState = RCC_HSE_OFF;
  RCC_OscInitStruct.LSEState = RCC_LSE_OFF;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 10;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) Error_Handler();

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK) Error_Handler();

}