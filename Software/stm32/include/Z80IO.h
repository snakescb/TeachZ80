/* -------------------------------------------------------------------------------------------------------
 Library  TeachZ80 IO-Requests 

 This library it coded to work together with the TeachZ80 Z80bus library.
 It can generate read and write IO Requests on the Z80 bus

 CAUTION: This code may change the output latches on the Z80. It cannot know the current status of the outputs.
 When the IO write requests are not used anymore (eg after SD access is completed), the Z80 should be reset
 
 Author: Christian Luethi
--------------------------------------------------------------------------------------------------------- */

#ifndef Z80_IO_H
#define Z80_IO_H

    #include <Arduino.h>
    #include <Z80Bus.h>

    void z80io_interrupt_config(void);

    class Z80IO  {
              
        public:            
            Z80IO(Z80Bus bus);    
            void ioreqHandler(uint16_t address, uint8_t data);
            void requestBus(bool request);
            void write(uint8_t ioport, uint8_t data);
            uint8_t read(uint8_t ioport);

        private:
            Z80Bus z80bus; 

            uint32_t counter;
    
    };

#endif