/* -------------------------------------------------------------------------------------------------------
 Board configuration

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

            Config(uint32_t flashPage = 127);            
            bool read(void);
            void write(void);
            void defaults(void);

        private:
            uint32_t flashPage;
            uint32_t flashAddress;
            //FLASH_PAGE_SIZE

    };

#endif