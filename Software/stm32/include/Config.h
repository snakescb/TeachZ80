/* -------------------------------------------------------------------------------------------------------
 Board configuration

 V1.2 boards have the stm32f722, with a secor based flash (rather than page based)
 The configuration is hardcoded to use sector 1, 16kb, address 0x08004000 - 0x08007FFF
 Requires the custom linker script to work, defining section ".config_flash" at this address

 Author: Christian Luethi
--------------------------------------------------------------------------------------------------------- */

#ifndef CONFIG_H
#define CONFIG_H

    #include <Arduino.h>

    class Config {

        public:

            struct clock_t {
                uint32_t z80Clock;
                uint32_t sioaClock;
                uint32_t siobClock;

            };

            struct  flash_t {
                uint32_t dumpStart;
                uint32_t dumpEnd;
            };
            

            struct configdata_t {
                uint8_t checksum;
                clock_t clock;    
                flash_t flash;            
            };

            configdata_t configdata;            

            Config();            
            bool read(void);
            void write(void);
            void defaults(void);

    };

#endif