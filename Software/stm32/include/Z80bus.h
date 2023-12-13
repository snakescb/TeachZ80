/* -------------------------------------------------------------------------------------------------------
 Library for Communication with the Z80 bus

 This library is not general purpose, it is hard coded to be used on the teachZ80 board
 
 Author: Christian Luethi
--------------------------------------------------------------------------------------------------------- */

#ifndef Z80BUS_H
#define Z80BUS_H

    #include <Arduino.h>

    #define IOREQ_PORT  0x60

    class Z80bus {

        enum Z80bus_mode { passive, active, ioreq_read, ioreq_write };
              
        public:            
            enum Z80bus_controlBits { reset, ioreq, mreq, rd, wr };

            Z80bus(void);    
            void resetZ80();
            void process(void);      
            bool request_bus();
            void release_bus();
            void release_dataBus();
            void release_addressBus();
            void release_controlBus();  
            void write_controlBit(Z80bus_controlBits bit, bool state); 
            void write_dataBus(uint8_t data);
            void write_addressBus(uint16_t address);
            uint8_t read_dataBus();
            uint16_t read_addressBus();    

        private:
            Z80bus_mode busmode;                                              
            uint32_t setPortBits(uint32_t portregister, uint32_t pinsInUnse, uint32_t bitalue, bool twoBits);   

    };

#endif