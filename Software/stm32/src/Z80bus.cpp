#include <Z80bus.h>

/* Types and definitions -------------------------------------------------------------------------------- */  

// These masks help for easy setup, write and release of pins later  
#define PORTA_BUS_LINES_IN_USE      0xF9C7    //on port A pins 0,1,2,6,7,8,11,12,13,14,15 are used by the bus
#define PORTA_DATA_LINES_IN_USE     0x00C7    //on port A pins 0,1,2,6,7 are used by the data bus
#define PORTA_CONTROL_LINES_IN_USE  0xF900    //on port A pins 8,11,12,13,14,15 are used by the control bus

#define PORTB_BUS_LINES_IN_USE      0xFFFF    //on port B all pins are used by the bus
#define PORTB_ADDRESS_LINES_IN_USE  0xFFFF    //on port B all pins are used by the address bus

#define PORTC_BUS_LINES_IN_USE      0xE000    //on port C pin 13,14,15 are used by the bus
#define PORTC_DATA_LINES_IN_USE     0xE000    //on port C pin 13,14,15 are used by the data bus

#define PORTH_BUS_LINES_IN_USE      0x0003    //on port H pin 0,1 are used by the bus
#define PORTH_CONTROL_LINES_IN_USE  0x0003    //on port H pin 0,1 are used by the control bus

//makros for reading control lines
#define WR_IS_HIGH       (GPIOA->IDR & GPIO_PIN_13)
#define RD_IS_HIGH       (GPIOA->IDR & GPIO_PIN_12)
#define WAIT_IS_HIGH     (GPIOA->IDR & GPIO_PIN_8)
#define MREQ_IS_HIGH     (GPIOA->IDR & GPIO_PIN_14)
#define IOREQ_IS_HIGH    (GPIOA->IDR & GPIO_PIN_15)
#define BUSREQ_IS_HIGH   (GPIOH->IDR & GPIO_PIN_0)
#define M1_IS_HIGH       (GPIOH->IDR & GPIO_PIN_1)
#define RESET_IS_HIGH    (GPIOA->IDR & GPIO_PIN_11)

//makros for setting control lines
#define WR_SET           GPIOA->BSRR = GPIO_PIN_13
#define RD_SET           GPIOA->BSRR = GPIO_PIN_12
#define WAIT_SET         GPIOA->BSRR =  GPIO_PIN_8
#define MREQ_SET         GPIOA->BSRR = GPIO_PIN_14
#define IOREQ_SET        GPIOA->BSRR = GPIO_PIN_15
#define BUSREQ_SET       GPIOH->BSRR = GPIO_PIN_0
#define M1_SET           GPIOH->BSRR = GPIO_PIN_1
#define RESET_SET        GPIOA->BSRR = GPIO_PIN_11

//makros for clearing (asserting) control lines
#define WR_CLR           GPIOA->BSRR = GPIO_PIN_13 << 16
#define RD_CLR           GPIOA->BSRR = GPIO_PIN_12 << 16
#define WAIT_CLR         GPIOA->BSRR =  GPIO_PIN_8 << 16
#define MREQ_CLR         GPIOA->BSRR = GPIO_PIN_14 << 16
#define IOREQ_CLR        GPIOA->BSRR = GPIO_PIN_15 << 16
#define BUSREQ_CLR       GPIOH->BSRR = GPIO_PIN_0 << 16
#define M1_CLR           GPIOH->BSRR = GPIO_PIN_1 << 16
#define RESET_CLR        GPIOA->BSRR = GPIO_PIN_11 << 16

/*--------------------------------------------------------------------------------------------------------
 process, run in main loop (polling mode)
---------------------------------------------------------------------------------------------------------*/
void Z80bus::process(void) {
    
    //check if there is an active IOREQ from the Z80
    //if so, check if it is for this device (on TeachZ80 only addresses 0x6x can be used by the stm32)
    if ((busmode == passive) && (!IOREQ_IS_HIGH) && (!M1_IS_HIGH)) { 
        uint16_t address = read_addressBus();
        if (address & 0x00F0 == IOREQ_PORT) {
            uint8_t ioregister = address & 0x000F;
            //check read or write access
            if (!WR_IS_HIGH) Serial.printf("IOREQ WRITE received to register %d\r\n", ioregister);
            if (!RD_IS_HIGH) Serial.printf("IOREQ READ received to register %d\r\n", ioregister);
        }
    }

}

/*--------------------------------------------------------------------------------------------------------
 Constructor
---------------------------------------------------------------------------------------------------------*/
Z80bus::Z80bus(void) {

    //I/O ports initialization
    //all lines required by the bus need to be open drain outputs, they have external pull ups to 5v
    busmode = passive;

    // 0x01 in OTYPER (1 bit per pin) sets the port pin to open drain mode
    GPIOA->OTYPER = setPortBits(GPIOA->OTYPER, PORTA_BUS_LINES_IN_USE, 0x01, false);
    GPIOB->OTYPER = setPortBits(GPIOB->OTYPER, PORTB_BUS_LINES_IN_USE, 0x01, false);
    GPIOC->OTYPER = setPortBits(GPIOC->OTYPER, PORTC_BUS_LINES_IN_USE, 0x01, false);
    GPIOH->OTYPER = setPortBits(GPIOH->OTYPER, PORTH_BUS_LINES_IN_USE, 0x01, false);

    // 0x01 in MODER (2 bits per pin) sets the port pin to output mode
    GPIOA->MODER = setPortBits(GPIOA->MODER, PORTA_BUS_LINES_IN_USE, 0x01, true);
    GPIOB->MODER = setPortBits(GPIOB->MODER, PORTB_BUS_LINES_IN_USE, 0x01, true);
    GPIOC->MODER = setPortBits(GPIOC->MODER, PORTC_BUS_LINES_IN_USE, 0x01, true);
    GPIOH->MODER = setPortBits(GPIOH->MODER, PORTH_BUS_LINES_IN_USE, 0x01, true);

    // 0x03 in OSPEEDR (2 bits per pin) sets the port pin to highspeed mode
    GPIOA->OSPEEDR = setPortBits(GPIOA->OSPEEDR, PORTA_BUS_LINES_IN_USE, 0x03, true);
    GPIOB->OSPEEDR = setPortBits(GPIOB->OSPEEDR, PORTB_BUS_LINES_IN_USE, 0x03, true);
    GPIOC->OSPEEDR = setPortBits(GPIOC->OSPEEDR, PORTC_BUS_LINES_IN_USE, 0x03, true);
    GPIOH->OSPEEDR = setPortBits(GPIOH->OSPEEDR, PORTH_BUS_LINES_IN_USE, 0x03, true);

    // 0x00 in PUPDR (2 bits per pin) disables the port pin pull-up/downs
    GPIOA->PUPDR = setPortBits(GPIOA->PUPDR, PORTA_BUS_LINES_IN_USE, 0x00, true);
    GPIOB->PUPDR = setPortBits(GPIOB->PUPDR, PORTB_BUS_LINES_IN_USE, 0x00, true);
    GPIOC->PUPDR = setPortBits(GPIOC->PUPDR, PORTC_BUS_LINES_IN_USE, 0x00, true);
    GPIOH->PUPDR = setPortBits(GPIOH->PUPDR, PORTH_BUS_LINES_IN_USE, 0x00, true);

    //release all lines
    release_dataBus();
    release_addressBus();
    release_controlBus();
}

/*--------------------------------------------------------------------------------------------------------
set/release a specific control bit
---------------------------------------------------------------------------------------------------------*/
void Z80bus::write_controlBit(Z80bus_controlBits bit, bool state) {
    if (busmode == passive) return;
    if ((bit == reset) &&   state) RESET_SET;
    if ((bit == reset) &&  !state) RESET_CLR;
}

/*--------------------------------------------------------------------------------------------------------
 request access to bus. returns false if bus is already active
 does not wait for Z80 to assert the busack line. It may be possible there is no CPU. Also, the CPU will
 always release the bus according the datasheet with high priority
---------------------------------------------------------------------------------------------------------*/
bool Z80bus::request_bus(){
    if (busmode != passive) return false;
    BUSREQ_CLR;
    busmode = active;
    return true;
}

/*--------------------------------------------------------------------------------------------------------
 Bus write functions
---------------------------------------------------------------------------------------------------------*/
void Z80bus::write_dataBus(uint8_t data) {
    if (busmode == passive) return;
    GPIOA->BSRR = (data & 0b11000111) << 16;
    GPIOH->BSRR = (data & 0b00111000) << 26; //yes, 26. 
}

void Z80bus::write_addressBus(uint8_t address) {
    if (busmode == passive) return;
    GPIOB->BSRR = address << 16;
}

/*--------------------------------------------------------------------------------------------------------
 Bus read functions
---------------------------------------------------------------------------------------------------------*/
uint8_t Z80bus::read_dataBus() {
    return (GPIOA->IDR & 0b11000111) | (GPIOH->IDR >> 10);
}

uint16_t Z80bus::read_addressBus() {
    return GPIOB->IDR;
}

/*--------------------------------------------------------------------------------------------------------
 Bus release functions - set output pins high (release pins)
---------------------------------------------------------------------------------------------------------*/
void Z80bus::release_dataBus() {
    GPIOA->BSRR = PORTA_DATA_LINES_IN_USE;
    GPIOC->BSRR = PORTC_DATA_LINES_IN_USE;
}

void Z80bus::release_addressBus() {
    GPIOB->BSRR = PORTB_ADDRESS_LINES_IN_USE;
}

void Z80bus::release_controlBus() {
    GPIOA->BSRR = PORTA_CONTROL_LINES_IN_USE;
    GPIOH->BSRR = PORTH_CONTROL_LINES_IN_USE;
    busmode = passive;
}

/*--------------------------------------------------------------------------------------------------------
Helper function to change bits in a 32bit gpio register based on a 16 bit mask (pins changed by this 
function) to a defined bitvalue. Works for 1 or two bits per pin
---------------------------------------------------------------------------------------------------------*/
uint32_t Z80bus::setPortBits(uint32_t portregister, uint32_t pinsInUnse, uint32_t bitvalue, bool twoBits) {
    uint32_t registerOut = 0;
    uint32_t checkmask = 0x01;
    uint32_t copymask = 0x01;
    if (twoBits) copymask = 0x03;
    uint32_t setmask = bitvalue & copymask;

    for (int i=0; i<15; i++) {
        if (pinsInUnse & checkmask) registerOut |= setmask; //set the bits according the bitvalue
        else registerOut |= (portregister & copymask);  //copy the bits from the input
        checkmask = checkmask << 1;
        setmask = setmask << 1;
        copymask = copymask << 1;
        if (twoBits) { 
            setmask = setmask << 1;
            copymask = copymask << 1;
        }
    }

    return registerOut;
}