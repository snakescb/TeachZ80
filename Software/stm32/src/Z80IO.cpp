#include <Z80IO.h>

/* Types and definitions -------------------------------------------------------------------------------- */  


extern Z80IO z80io;
/*--------------------------------------------------------------------------------------------------------
EXTI interrupt handler to handle IORequests
To be as fast as possible, change interupt priority through HAL
Also, avoid the HAL / Arduino default handlers to be called, by redefining EXTI9_5_IRQHandler

On V1.0 / V1.1 boards with the stm32l433, reading iorequests works reliably until 6MHz only

Requires to add __weak to void EXTI9_5_IRQHandler(void) in
(user folder)\.platformio\packages\framework-arduinoststm32\libraries\SrcWrapper\src\stm32\interrupt.cpp
---------------------------------------------------------------------------------------------------------*/
void z80io_interrupt_config(void) {
    //setup iroeq interrupt
    HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
    HAL_NVIC_SetPriority(SysTick_IRQn, 1, 0);
    attachInterrupt(PA8, 0, FALLING);
}

extern "C"  {   
    void EXTI9_5_IRQHandler(void){
        uint16_t gpioB = GPIOB->IDR;
        uint16_t gpioA = GPIOA->IDR;
        uint16_t gpioC = GPIOC->IDR;

        uint8_t data = (gpioA & 0x00C7) | ((gpioC & 0xE000) >> 10);           
        z80io.ioreqHandler(gpioB & 0xFF, data);

        //reset the interrupt flag
        EXTI->PR1 = EXTI_PR1_PIF8; 

    }
}

/*--------------------------------------------------------------------------------------------------------
 Constructor
---------------------------------------------------------------------------------------------------------*/
Z80IO::Z80IO(Z80Bus bus) : z80bus(bus) {
    counter = 0;
}

/*--------------------------------------------------------------------------------------------------------
 Handler Function
---------------------------------------------------------------------------------------------------------*/
void Z80IO::ioreqHandler(uint16_t address, uint8_t data) {
    counter++;
    //Serial.printf("Counter: %u - Address: %u - Data: %u\r\n", counter, address, data);
}

/*--------------------------------------------------------------------------------------------------------
 IO Write Access to the bus
---------------------------------------------------------------------------------------------------------*/
void Z80IO::write(uint8_t ioport, uint8_t data) {
    z80bus.write_addressBus(ioport);
    z80bus.write_dataBus(data);
    z80bus.write_controlBit(z80bus.ioreq, false);
    z80bus.write_controlBit(z80bus.wr, false);
    z80bus.write_controlBit(z80bus.wr, true);
    z80bus.write_controlBit(z80bus.ioreq, true);
    z80bus.release_addressBus();
    z80bus.release_dataBus();
}

/*--------------------------------------------------------------------------------------------------------
 IO Read Access to the bus
---------------------------------------------------------------------------------------------------------*/
uint8_t Z80IO::read(uint8_t ioport) {
    z80bus.write_addressBus(ioport);
    z80bus.write_controlBit(z80bus.ioreq, false);
    z80bus.write_controlBit(z80bus.rd, false);
    delayMicroseconds(1);
    uint8_t data = z80bus.read_dataBus();
    z80bus.write_controlBit(z80bus.rd, true);
    z80bus.write_controlBit(z80bus.ioreq, true);
    z80bus.release_addressBus();
    return data;
}

/*--------------------------------------------------------------------------------------------------------
 Request the Z80 bus
---------------------------------------------------------------------------------------------------------*/
void Z80IO::requestBus(bool request) {
    if (request) z80bus.request_bus();
    else z80bus.release_bus();
}
  