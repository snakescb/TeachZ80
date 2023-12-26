#include <Z80Bus.h>

/* Types and definitions -------------------------------------------------------------------------------- */  

// These masks help for easy setup, write and release of pins later  
#define PORTA_BUS_LINES_IN_USE      0xF9C7    //on port A pins 0,1,2,6,7,8,11,12,13,14,15 are used by the bus
#define PORTA_DATA_LINES_IN_USE     0x00C7    //on port A pins 0,1,2,6,7 are used by the data bus
#define PORTA_CONTROL_LINES_IN_USE  0xF900    //on port A pins 8,11,12,13,14,15 are used by the control bus
#define PORTA_CONTROL_LINES_EX_RES  0xF100  //same, but excluding reset line

#define PORTB_BUS_LINES_IN_USE      0xFFFF    //on port B all pins are used by the bus
#define PORTB_ADDRESS_LINES_IN_USE  0xFFFF    //on port B all pins are used by the address bus

#define PORTC_BUS_LINES_IN_USE      0xE000    //on port C pin 13,14,15 are used by the bus
#define PORTC_DATA_LINES_IN_USE     0xE000    //on port C pin 13,14,15 are used by the data bus

#define PORTH_BUS_LINES_IN_USE      0x0003    //on port H pin 0,1 are used by the bus
#define PORTH_CONTROL_LINES_IN_USE  0x0003    //on port H pin 0,1 are used by the control bus

//timings
#define RESET_PULSE_LEN_ms  50

//makros for reading control lines
#define WR_IS_HIGH       (GPIOA->IDR & GPIO_PIN_13)
#define RD_IS_HIGH       (GPIOA->IDR & GPIO_PIN_12)
#define WAIT_IS_HIGH     (GPIOH->IDR & GPIO_PIN_1)
#define MREQ_IS_HIGH     (GPIOA->IDR & GPIO_PIN_14)
#define IOREQ_IS_HIGH    (GPIOA->IDR & GPIO_PIN_15)
#define BUSREQ_IS_HIGH   (GPIOH->IDR & GPIO_PIN_0)
#define RESET_IS_HIGH    (GPIOA->IDR & GPIO_PIN_11)

//makros for setting control lines
#define WR_SET           GPIOA->BSRR = GPIO_PIN_13
#define RD_SET           GPIOA->BSRR = GPIO_PIN_12
#define WAIT_SET         GPIOH->BSRR = GPIO_PIN_1
#define MREQ_SET         GPIOA->BSRR = GPIO_PIN_14
#define IOREQ_SET        GPIOA->BSRR = GPIO_PIN_15
#define BUSREQ_SET       GPIOH->BSRR = GPIO_PIN_0
#define RESET_SET        GPIOA->BSRR = GPIO_PIN_11

//makros for clearing (asserting) control lines
#define WR_CLR           GPIOA->BSRR = GPIO_PIN_13 << 16
#define RD_CLR           GPIOA->BSRR = GPIO_PIN_12 << 16
#define WAIT_CLR         GPIOH->BSRR = GPIO_PIN_1 << 16
#define MREQ_CLR         GPIOA->BSRR = GPIO_PIN_14 << 16
#define IOREQ_CLR        GPIOA->BSRR = GPIO_PIN_15 << 16
#define BUSREQ_CLR       GPIOH->BSRR = GPIO_PIN_0 << 16
#define RESET_CLR        GPIOA->BSRR = GPIO_PIN_11 << 16

/*--------------------------------------------------------------------------------------------------------
 Constructor
---------------------------------------------------------------------------------------------------------*/
Z80Bus::Z80Bus(void) { 
    //I/O ports initialization
    //all lines required by the bus need to be open drain outputs, they have external pull ups to 5v
    busmode = passive;

    //Enable GPIO clocks in case they have not by the arduino framework
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();

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
    release_bus();
}

/*--------------------------------------------------------------------------------------------------------
This function enable or disables push-pull on the control pins (excluding reset). 
This is required for consistent memory programming
---------------------------------------------------------------------------------------------------------*/
void Z80Bus::controlPinsActiveDrive(bool enable) {
    if (enable) {
        // 0x00 in OTYPER (1 bit per pin) sets the port pin to push-pull
        GPIOA->OTYPER = setPortBits(GPIOA->OTYPER, PORTA_CONTROL_LINES_EX_RES, 0x00, false);
        GPIOH->OTYPER = setPortBits(GPIOH->OTYPER, PORTH_BUS_LINES_IN_USE, 0x00, false);
    }
    else {
        // 0x01 in OTYPER (1 bit per pin) sets the port pin to open-drain
        GPIOA->OTYPER = setPortBits(GPIOA->OTYPER, PORTA_CONTROL_LINES_EX_RES, 0x01, false);
        GPIOH->OTYPER = setPortBits(GPIOH->OTYPER, PORTH_BUS_LINES_IN_USE, 0x01, false);
    }
}

/*--------------------------------------------------------------------------------------------------------
reset Z80
---------------------------------------------------------------------------------------------------------*/
void Z80Bus::resetZ80() {
    RESET_SET;
    delay(RESET_PULSE_LEN_ms);
    RESET_CLR;
}

/*--------------------------------------------------------------------------------------------------------
set/release a specific control bit
---------------------------------------------------------------------------------------------------------*/
void Z80Bus::write_controlBit(Z80Bus_controlBits bit, bool state) {    
    //reset can be controlled anytime, other lines only when bus is actively controlled
    if ((bit == reset ) &&  state) RESET_SET;
    if ((bit == reset ) && !state) RESET_CLR;
    if (busmode == passive) return;
    if ((bit == ioreq ) &&  state) IOREQ_SET;
    if ((bit == ioreq ) && !state) IOREQ_CLR;
    if ((bit == mreq  ) &&  state) MREQ_SET;
    if ((bit == mreq  ) && !state) MREQ_CLR;
    if ((bit == rd    ) &&  state) RD_SET;
    if ((bit == rd    ) && !state) RD_CLR;
    if ((bit == wr    ) &&  state) WR_SET;
    if ((bit == wr    ) && !state) WR_CLR;
    if ((bit == wait  ) &&  state) WAIT_SET;
    if ((bit == wait  ) && !state) WAIT_CLR;
}

/*--------------------------------------------------------------------------------------------------------
 Bus write functions
---------------------------------------------------------------------------------------------------------*/
void Z80Bus::write_dataBus(uint8_t data) {
    if (busmode == passive) return;
    uint16_t dataA = data & PORTA_DATA_LINES_IN_USE;
    uint16_t dataC = (data << 10) & PORTC_DATA_LINES_IN_USE;

    GPIOA->BSRR = data & PORTA_DATA_LINES_IN_USE;
    GPIOA->BSRR = (~dataA & PORTA_DATA_LINES_IN_USE) << 16;
    GPIOC->BSRR = (data << 10) & PORTC_DATA_LINES_IN_USE;
    GPIOC->BSRR = (~(data << 10) & PORTC_DATA_LINES_IN_USE) << 16;
}

void Z80Bus::write_addressBus(uint16_t address) {
    if (busmode == passive) return;
    GPIOB->ODR = address;
}

/*--------------------------------------------------------------------------------------------------------
 Bus read functions
---------------------------------------------------------------------------------------------------------*/
uint8_t Z80Bus::read_dataBus() {
    return (GPIOA->IDR & PORTA_DATA_LINES_IN_USE) | ((GPIOC->IDR & PORTC_DATA_LINES_IN_USE) >> 10);
}

uint16_t Z80Bus::read_addressBus() {
    return GPIOB->IDR;
}

/*--------------------------------------------------------------------------------------------------------
 request access to bus. returns false if bus is already active
 does not wait for Z80 to assert the busack line. It may be possible there is no CPU. Also, the CPU will
 always release the bus according the datasheet with high priority
---------------------------------------------------------------------------------------------------------*/
bool Z80Bus::request_bus(){
    if (busmode != passive) return false;
    release_bus();
    BUSREQ_CLR;
    delayMicroseconds(10);
    controlPinsActiveDrive(true);
    busmode = active;
    return true;
}

/*--------------------------------------------------------------------------------------------------------
 Bus release functions - set output pins high (release pins)
---------------------------------------------------------------------------------------------------------*/
void Z80Bus::release_bus() {
    controlPinsActiveDrive(false);
    release_dataBus();
    release_addressBus();
    release_controlBus();
    BUSREQ_SET;
    busmode = passive;
}

void Z80Bus::release_dataBus() {
    GPIOA->BSRR = PORTA_DATA_LINES_IN_USE;
    GPIOC->BSRR = PORTC_DATA_LINES_IN_USE;
}

void Z80Bus::release_addressBus() {
    GPIOB->BSRR = PORTB_ADDRESS_LINES_IN_USE;
}

void Z80Bus::release_controlBus() {
    GPIOA->BSRR = PORTA_CONTROL_LINES_EX_RES;
    GPIOH->BSRR = PORTH_CONTROL_LINES_IN_USE;
}

/*--------------------------------------------------------------------------------------------------------
Helper function to change bits in a 32bit gpio register based on a 16 bit mask (pins changed by this 
function) to a defined bitvalue. Works for 1 or 2 bits per pin
---------------------------------------------------------------------------------------------------------*/
uint32_t Z80Bus::setPortBits(uint32_t portregister, uint32_t pinsInUnse, uint32_t bitvalue, bool twoBits) {
    uint32_t registerOut = 0;
    uint32_t checkmask = 0x01;
    uint32_t copymask = 0x01;
    if (twoBits) copymask = 0x03;
    uint32_t setmask = bitvalue & copymask;

    for (int i=0; i<16; i++) {
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