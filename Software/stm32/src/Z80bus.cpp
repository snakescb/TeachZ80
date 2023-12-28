#include <Z80Bus.h>

/* Types and definitions -------------------------------------------------------------------------------- */  

// These masks help for easy setup, write and release of pins later  
#define PORTA_BUS_LINES_IN_USE      0x80C9      //on port A pins 0,3,6,7,15 are used by the bus
#define PORTA_CONTROL_LINES_IN_USE  0x80C9      //on port A pins 0,3,6,7,15 are used by the control bus
#define PORTA_CONTROL_LINES_EX_RES  0x80C8      //same, but excluding reset line

#define PORTB_BUS_LINES_IN_USE      0xFFFF      //on port B all pins are used by the bus
#define PORTB_ADDRESS_LINES_IN_USE  0xFFFF      //on port B all pins are used by the address bus

#define PORTC_BUS_LINES_IN_USE      0x0FCF      //on port C pin 0,1,2,3,6,7,8,9,10,11 used by the bus
#define PORTC_DATA_LINES_IN_USE     0x03CF      //on port C pin 0,1,2,3,6,7,8,9 used by the data bus
#define PORTC_CONTROL_LINES_IN_USE  0x0C00      //on port C pin 10,11 used by the control bus

//timings
#define RESET_PULSE_LEN_ms  100

//makros for reading control lines
#define WR_IS_HIGH       (GPIOC->IDR & GPIO_PIN_10)
#define RD_IS_HIGH       (GPIOA->IDR & GPIO_PIN_15)
#define WAIT_IS_HIGH     (GPIOA->IDR & GPIO_PIN_7)
#define MREQ_IS_HIGH     (GPIOC->IDR & GPIO_PIN_11)
#define IOREQ_IS_HIGH    (GPIOA->IDR & GPIO_PIN_3)
#define BUSREQ_IS_HIGH   (GPIOA->IDR & GPIO_PIN_6)
#define RESET_IS_HIGH    (GPIOA->IDR & GPIO_PIN_0)

//makros for setting control lines
#define WR_SET           GPIOC->BSRR = GPIO_PIN_10
#define RD_SET           GPIOA->BSRR = GPIO_PIN_15
#define WAIT_SET         GPIOA->BSRR = GPIO_PIN_7
#define MREQ_SET         GPIOC->BSRR = GPIO_PIN_11
#define IOREQ_SET        GPIOA->BSRR = GPIO_PIN_3
#define BUSREQ_SET       GPIOA->BSRR = GPIO_PIN_6
#define RESET_SET        GPIOA->BSRR = GPIO_PIN_0

//makros for clearing (asserting) control lines
#define WR_CLR           GPIOC->BSRR = GPIO_PIN_10 << 16
#define RD_CLR           GPIOA->BSRR = GPIO_PIN_15 << 16
#define WAIT_CLR         GPIOA->BSRR = GPIO_PIN_7 << 16
#define MREQ_CLR         GPIOC->BSRR = GPIO_PIN_11 << 16
#define IOREQ_CLR        GPIOA->BSRR = GPIO_PIN_3 << 16
#define BUSREQ_CLR       GPIOA->BSRR = GPIO_PIN_6 << 16
#define RESET_CLR        GPIOA->BSRR = GPIO_PIN_0 << 16

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

    // 0x01 in OTYPER (1 bit per pin) sets the port pin to open drain mode
    GPIOA->OTYPER = setPortBits(GPIOA->OTYPER, PORTA_BUS_LINES_IN_USE, 0x01, false);
    GPIOB->OTYPER = setPortBits(GPIOB->OTYPER, PORTB_BUS_LINES_IN_USE, 0x01, false);
    GPIOC->OTYPER = setPortBits(GPIOC->OTYPER, PORTC_BUS_LINES_IN_USE, 0x01, false);

    // 0x01 in MODER (2 bits per pin) sets the port pin to output mode
    GPIOA->MODER = setPortBits(GPIOA->MODER, PORTA_BUS_LINES_IN_USE, 0x01, true);
    GPIOB->MODER = setPortBits(GPIOB->MODER, PORTB_BUS_LINES_IN_USE, 0x01, true);
    GPIOC->MODER = setPortBits(GPIOC->MODER, PORTC_BUS_LINES_IN_USE, 0x01, true);

    // 0x03 in OSPEEDR (2 bits per pin) sets the port pin to highspeed mode
    GPIOA->OSPEEDR = setPortBits(GPIOA->OSPEEDR, PORTA_BUS_LINES_IN_USE, 0x03, true);
    GPIOB->OSPEEDR = setPortBits(GPIOB->OSPEEDR, PORTB_BUS_LINES_IN_USE, 0x03, true);
    GPIOC->OSPEEDR = setPortBits(GPIOC->OSPEEDR, PORTC_BUS_LINES_IN_USE, 0x03, true);

    // 0x00 in PUPDR (2 bits per pin) disables the port pin pull-up/downs
    GPIOA->PUPDR = setPortBits(GPIOA->PUPDR, PORTA_BUS_LINES_IN_USE, 0x00, true);
    GPIOB->PUPDR = setPortBits(GPIOB->PUPDR, PORTB_BUS_LINES_IN_USE, 0x00, true);
    GPIOC->PUPDR = setPortBits(GPIOC->PUPDR, PORTC_BUS_LINES_IN_USE, 0x00, true);

    //release all lines
    RESET_SET;
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
        GPIOC->OTYPER = setPortBits(GPIOC->OTYPER, PORTC_CONTROL_LINES_IN_USE, 0x00, false);
    }
    else {
        // 0x01 in OTYPER (1 bit per pin) sets the port pin to open-drain
        GPIOA->OTYPER = setPortBits(GPIOA->OTYPER, PORTA_CONTROL_LINES_EX_RES, 0x01, false);
        GPIOC->OTYPER = setPortBits(GPIOC->OTYPER, PORTC_CONTROL_LINES_IN_USE, 0x01, false);
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
    delayMicroseconds(1);   
}

/*--------------------------------------------------------------------------------------------------------
 Bus write functions
---------------------------------------------------------------------------------------------------------*/
void Z80Bus::write_dataBus(uint8_t data) {
    if (busmode == passive) return;
    GPIOC->BSRR = (PORTC_DATA_LINES_IN_USE) << 16;
    GPIOC->BSRR = (data & 0x0F) | ((data & 0xF0) << 2);
    delayMicroseconds(1);   //required on the fast processor for bus to stabilize
}

void Z80Bus::write_addressBus(uint16_t address) {
    if (busmode == passive) return;
    GPIOB->ODR = address;
    delayMicroseconds(1);   //required on the fast processor for bus to stabilize
}

/*--------------------------------------------------------------------------------------------------------
 Bus read functions
---------------------------------------------------------------------------------------------------------*/
uint8_t Z80Bus::read_dataBus() {
    return ((GPIOC->IDR & 0x3C0) >> 2) | (GPIOC->IDR & 0x0F);
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
    GPIOC->BSRR = PORTC_DATA_LINES_IN_USE;
}

void Z80Bus::release_addressBus() {
    GPIOB->BSRR = PORTB_ADDRESS_LINES_IN_USE;
}

void Z80Bus::release_controlBus() {
    GPIOA->BSRR = PORTA_CONTROL_LINES_EX_RES;
    GPIOC->BSRR = PORTC_CONTROL_LINES_IN_USE;
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