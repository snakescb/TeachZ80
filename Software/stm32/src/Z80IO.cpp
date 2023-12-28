#include <Z80IO.h>

/* Types and definitions -------------------------------------------------------------------------------- */  

extern Z80IO z80io;
/*--------------------------------------------------------------------------------------------------------
Setup the IOREQ_60 interrupt and changes it ho highest priority
---------------------------------------------------------------------------------------------------------*/
void z80io_interrupt_config(void) {
    //setup iroeq60 interrupt
    HAL_NVIC_SetPriority(EXTI1_IRQn, 0, 0);
    HAL_NVIC_SetPriority(SysTick_IRQn, 1, 0);

    GPIO_InitTypeDef gpioInit;
    gpioInit.Pin = GPIO_PIN_1;
    gpioInit.Mode = GPIO_MODE_IT_FALLING;
    gpioInit.Pull = GPIO_NOPULL;
    gpioInit.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

    HAL_GPIO_Init(GPIOA, &gpioInit);
    HAL_NVIC_EnableIRQ(EXTI1_IRQn);
}

/*--------------------------------------------------------------------------------------------------------
EXTI interrupt handler
To be as fast as possible, it will be executed from RAM
Also, it redefines the default HAL/Arduino interrupt handler
Requires to add __weak to void EXTI1_IRQHandler(void) in
(user folder)\.platformio\packages\framework-arduinoststm32\libraries\SrcWrapper\src\stm32\interrupt.cpp
---------------------------------------------------------------------------------------------------------*/
extern "C"  {      
    //__attribute__ ((long_call, section (".RamFunc"))) void EXTI1_IRQHandler(void) {
    void EXTI1_IRQHandler(void) {
        GPIOA->BSRR = GPIO_PIN_7 << 16;   //clear wait
        //z80io.irqaddress = GPIOB->IDR & 0xFF;
        //z80io.irqdata = ((GPIOC->IDR & 0x3C0) >> 2) | (GPIOC->IDR & 0x0F);
        GPIOA->BSRR = GPIO_PIN_7;         //set wait                        
        EXTI->PR = GPIO_PIN_1;            // reset exti flag
    }
}

/*--------------------------------------------------------------------------------------------------------
 Constructor
---------------------------------------------------------------------------------------------------------*/
Z80IO::Z80IO(Z80Bus bus) : z80bus(bus) {
    irqcounter = 0;
    irqaddress = 0;
    irqdata = 0;
}

/*--------------------------------------------------------------------------------------------------------
 process function
---------------------------------------------------------------------------------------------------------*/
void Z80IO::process(void) {
    if (irqaddress != 0) {
        irqcounter++;
        //Serial.printf("Counter: %u - Address: %u - Data: %u\r\n", irqcounter, irqaddress, irqdata);
        irqaddress = 0;
    }
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
  