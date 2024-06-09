/* -------------------------------------------------------------------------------------------------------
 teachZ80 Support processor firmware

 Author   : Christian Luethi
--------------------------------------------------------------------------------------------------------- */
#include <Arduino.h>
#include <Led.h>
#include <Button.h>
#include <Console.h>
#include <Si5153.h>
#include <Config.h>
#include <Z80Bus.h>
#include <Z80Flash.h>
#include <Z80IO.h>
#include <Z80SPI.h>
#include <Z80SDCard.h>
#include <CPMFileSystem.h>
#include <FlashLoader.h>
#include <Bootloader.h>

/* Types and definitions -------------------------------------------------------------------------------- */
// Startup delay
#define STARTUP_DELAY_ms	 500

// In idle, one quick blink every 5 seconds
#define IDLE_LED_ON_PERIOD	   10
#define IDLE_LED_OFF_PERIOD  4990

// In Flash Mode, one second blinks
#define FLASHMODE_LED_ON_PERIOD	  500
#define FLASHMODE_LED_OFF_PERIOD  500

enum ApplicationState : byte { Idle, FlashMode };

/* Variables and instances ------------------------------------------------------------------------------ */
ApplicationState applicationState ;
Led statusLed(PA2);
Button button(PD2);
Si5153 clock(PA11, PA12);
Config config;
Console console;
Z80Bus z80bus;
Z80IO z80io(z80bus);
Z80SPI z80spi(z80io);
Z80SDCard z80sdcard(z80spi);
Z80Flash z80flash(z80bus);
FlashLoader flashloader(z80flash);
CPMFileSystem filesystem(CPMFileSystem::geometry_8k_8m_32_512, z80sdcard, z80bus);

/*#########################################################################################################
 Main Program5
##########################################################################################################*/

/* Initialization --------------------------------------------------------------------------------------- */
void setup() {
	
	Serial.setTx(PA9);
	Serial.setRx(PA10);	
	Serial.begin(115200);
	statusLed.on();

	if (!config.read()) config.defaults();

	if (clock.begin()) {	
		clock.configureChannel(0, config.configdata.clock.z80Clock, clock.DIV1, clock.PLLA, clock.ENABLE);   //10.000 MHz System Cock
		clock.configureChannel(1, config.configdata.clock.sioaClock, clock.DIV1, clock.PLLB, clock.ENABLE);  //1.8432 MHZ Clock SIOA
		clock.configureChannel(2, config.configdata.clock.siobClock, clock.DIV1, clock.PLLB, clock.ENABLE);  //1.8432 MHZ Clock SIOB
	}

	console.begin();
	z80bus.resetZ80();
	delay(STARTUP_DELAY_ms);
	
	applicationState = Idle;
	statusLed.set(IDLE_LED_ON_PERIOD, IDLE_LED_OFF_PERIOD);

	
}

/* Main loop -------------------------------------------------------------------------------------------- */
void loop() {
   
	//Process modules
	flashloader.process();
   	statusLed.process();
   	button.process();

	//Process Serial Data
	int rx = Serial.read();	
	if (rx != -1){
		if (!flashloader.serialUpdate((uint8_t) rx)) {			
			bootloader_magicSentence((uint8_t) rx);
			console.serialUpdate((uint8_t) rx);
		}
	}

	//enable flash mode with button pushed
	if (button.pushTrigger()) flashloader.setMode(true);

	//led control
	if ((applicationState == Idle) && (flashloader.loadermode == flashloader.active)) {
		statusLed.set(FLASHMODE_LED_ON_PERIOD, FLASHMODE_LED_OFF_PERIOD);
		applicationState = FlashMode;
	}
	if ((applicationState == FlashMode) && (flashloader.loadermode == flashloader.inactive)) {
		statusLed.set(IDLE_LED_ON_PERIOD, IDLE_LED_OFF_PERIOD);
		applicationState = Idle;
	}
}

/* -------------------------------------------------------------------------------------------------------
 STM32CubeMX Generated Clock Initialization, 80MHz Systemclock
--------------------------------------------------------------------------------------------------------- */
void SystemClock_Config(void) {

	//initialize clocks
	RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  	RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

 	/** Configure the main internal regulator output voltage */
  	__HAL_RCC_PWR_CLK_ENABLE();
  	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  	/** Initializes the RCC Oscillators according to the specified parameters in the RCC_OscInitTypeDef structure. */
  	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  	RCC_OscInitStruct.PLL.PLLM = 8;
  	RCC_OscInitStruct.PLL.PLLN = 216;
  	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  	RCC_OscInitStruct.PLL.PLLQ = 2;
  	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) { Error_Handler(); }

  	/** Activate the Over-Drive mode */
  	if (HAL_PWREx_EnableOverDrive() != HAL_OK) { Error_Handler(); }

  	/** Initializes the CPU, AHB and APB buses clocks */
  	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_7) != HAL_OK) { Error_Handler(); }

}
