/* -------------------------------------------------------------------------------------------------------
 Library for Communication with the Z80 bus

 This library is not general purpose, it is hard coded to be used on the teachZ80 board
 
 Author: Christian Luethi
--------------------------------------------------------------------------------------------------------- */

#ifndef Z80_BUS_H
#define Z80_BUS_H

    #include <Arduino.h>

    class Z80Bus {                     
              
        public:    
            enum Z80Bus_controlBits { reset, ioreq, mreq, rd, wr, wait };   

            Z80Bus(void);    
            void resetZ80();     
            bool request_bus();
            void release_bus();
            void release_dataBus();
            void release_addressBus();
            void release_controlBus();  
            void write_controlBit(Z80Bus_controlBits bit, bool state); 
            void write_dataBus(uint8_t data);
            void write_addressBus(uint16_t address);
            uint8_t read_dataBus();
            uint16_t read_addressBus();   

        private:         
            enum Z80Bus_mode { passive, active }; 
            Z80Bus_mode busmode;       

            uint32_t setPortBits(uint32_t portregister, uint32_t pinsInUnse, uint32_t bitalue, bool twoBits);  
            void controlPinsActiveDrive(bool enable);
            static void ioreq_handler(void);

    };

#endif