/* -------------------------------------------------------------------------------------------------------
 Library  TeachZ80 SPI

 This library it coded to work together with the TeachZ80 Z80IO library.
 It is using IO requests to create an SPI bus used for accessing the SD card, just like the
 Z80 board is doing it as well.

 This library implements SPI mode 0 (SD cards operate on SPI mode 0.)
 Data changes on falling CLK edge & sampled on rising CLK edge:
        __                                             ___
 /SSEL    \______________________ ... ________________/      Host --> Device
                 __    __    __   ... _    __    __
 CLK    ________/  \__/  \__/  \__     \__/  \__/  \______   Host --> Device
        _____ _____ _____ _____ _     _ _____ _____ ______
 MOSI        \_____X_____X_____X_ ... _X_____X_____/         Host --> Device
        _____ _____ _____ _____ _     _ _____ _____ ______
 MISO        \_____X_____X_____X_ ... _X_____X_____/         Host <-- Device
 
 Author: Christian Luethi
--------------------------------------------------------------------------------------------------------- */

#ifndef Z80_SPI_H
#define Z80_SPI_H

    #include <Arduino.h>
    #include <Z80IO.h>

    class Z80SPI  {
              
        public:            
            Z80SPI(Z80IO io);   
            void requestBus(bool request);
            void slaveSelect(bool state);
            void writeByte(uint8_t data); 
            uint8_t readByte(void); 
            bool checkSDDetect(void);

        private:
            Z80IO z80io;             
            uint8_t outputBuffer;    
    };

#endif