#include <Z80IO.h>

/* Types and definitions -------------------------------------------------------------------------------- */  



extern Z80IO z80io;
/*--------------------------------------------------------------------------------------------------------
Setup the IOREQ_60 interrupt and changes it ho highest priority.
Relocates the interrupt table from Flash to RAM
---------------------------------------------------------------------------------------------------------*/
void z80io_interrupt_config(void) {
    
    //Disable interrupts
    __disable_irq();

    //setup and enable iroeq60 interrupt
    GPIO_InitTypeDef gpioInit;
    gpioInit.Pin = GPIO_PIN_8;
    gpioInit.Mode = GPIO_MODE_IT_FALLING;
    gpioInit.Pull = GPIO_NOPULL;
    gpioInit.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

    HAL_GPIO_Init(GPIOA, &gpioInit);
    HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

    //Adjust IRQ priorities
    HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
    HAL_NVIC_SetPriority(SysTick_IRQn, 1, 0);

    //Copy interrupt table from FLASH to RAM (128*4 bytes fit in the 512 area left open in the linker script)
    uint32_t* psrc = (uint32_t*) FLASH_BASE; 
    uint32_t* pdst = (uint32_t*) SRAM_BASE;
    for (int i=0; i<128; i++) pdst[i] = psrc[i];

    //repoint vector table to new area in RAM
    SCB->VTOR = SRAM_BASE;

    //Enable interrupts
    __enable_irq();
}

/*--------------------------------------------------------------------------------------------------------
EXTI interrupt handler
To be as fast as possible, the ISR handler will be executed from RAM. Also, the init function has moved the 
interrupt table from Flash to RAM. Last but not least, it redefines the default HAL/Arduino interrupt handler.
This requires to add __weak to void EXTI1_IRQHandler(void) in
(user folder)\.platformio\packages\framework-arduinoststm32\libraries\SrcWrapper\src\stm32\interrupt.cpp
---------------------------------------------------------------------------------------------------------*/
extern "C"  {   
    __attribute__ ((long_call, section (".RamFunc"))) void EXTI9_5_IRQHandler(void){
        z80io.irqPortA = GPIOA->IDR;
        z80io.irqPortB = GPIOB->IDR;
        z80io.irqPortC = GPIOC->IDR; 
        //reset the interrupt flag
        EXTI->PR1 = EXTI_PR1_PIF8; 
    }
}

/*--------------------------------------------------------------------------------------------------------
 Constructor
---------------------------------------------------------------------------------------------------------*/
Z80IO::Z80IO(Z80Bus bus) : z80bus(bus) {
}

/*--------------------------------------------------------------------------------------------------------
 process function
---------------------------------------------------------------------------------------------------------*/
void Z80IO::process(void) {
    if (irqPortB != 0) {

        //Decode ports received by the interrupt handler
        uint8_t address =  irqPortB;
        uint8_t data    =  (irqPortA & 0x00C7) | ((irqPortC & 0xE000) >> 10);     

        Serial.printf("Address: %04X - Data: %02X\r\n", address, data);
        irqPortB = 0;
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
  