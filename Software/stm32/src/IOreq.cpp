#include <IOreq.h>

/* Types and definitions -------------------------------------------------------------------------------- */  

/*--------------------------------------------------------------------------------------------------------
 Constructor
---------------------------------------------------------------------------------------------------------*/
IOreq::IOreq() {
    counter = 0;
}

/*--------------------------------------------------------------------------------------------------------
 Handler Function
---------------------------------------------------------------------------------------------------------*/
void IOreq::ioreqHandler(uint16_t address, uint8_t data) {
    counter++;
    //Serial.printf("Counter: %u - Address: %u - Data: %u\r\n", counter, address, data);
}
  