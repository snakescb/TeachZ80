/* -------------------------------------------------------------------------------------------------------
 Handling the Z80 Bus IO-Requests

 Author: Christian Luethi
--------------------------------------------------------------------------------------------------------- */

#ifndef IOREQ_H
#define IOREQ_H

    #include <Arduino.h>

    class IOreq  {
              
        public:            
            IOreq(void);    
            void ioreqHandler(uint16_t address, uint8_t data);
            uint32_t counter;
    
    };

#endif