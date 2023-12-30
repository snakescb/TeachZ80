/* -------------------------------------------------------------------------------------------------------
 Library  TeachZ80 SD Card Library

 This library can access the TeachZ80 SD card, through the Z80 SPI driver, through Z80 IO Requests

 Thi is not a complete SD library, it is very much simplified based on what is required for TeachZ80
 This library is implemented based on John Winans original SD library in the Z80, written in asm
 
 Author: Christian Luethi
--------------------------------------------------------------------------------------------------------- */

#ifndef Z80_SD_H
#define Z80_SD_H

    #define SD_COMMAND_BUFFER_LENGTH          6
    #define SD_BLOCK_SIZE                   512

    #include <Arduino.h>
    #include <Z80SPI.h>
    #include <Z80IO.h>

    class Z80SDCard  {

        public:        
            enum sdResult: uint8_t { ok, nocard, not_idle, invalid_status, not_ready, invalid_capacity, read_timeout, write_error, write_timeout_1, write_timeout_2, invalid_partition }; 
            
            struct partition_t { 
                uint32_t block; 
                uint32_t size; 
                uint8_t type;
                uint8_t status;
            };
            
            struct mbrResult { 
                sdResult accessresult; 
                sdResult readresult; 
                uint8_t partitions;
                partition_t partitiontable[4]; 
            };

            Z80SDCard(Z80SPI spi);   
            sdResult accessCard(bool state);      
            sdResult readBlock(uint32_t blockNumber, uint8_t* dst);
            sdResult writeBlock(uint32_t blockNumber, uint8_t* src);
            mbrResult readMBR();
            sdResult formatCard();
            sdResult writeProgram(uint8_t partition, uint8_t programNumber);
            uint8_t sdDataBuffer[SD_BLOCK_SIZE];

        private:
            void wakeupCard();
            void selectCard(bool select);
            void sdCommand(uint8_t* cmd, uint8_t txlen, uint8_t rxlen, uint8_t maxtries = 15, bool controlssel = true);
            void parseMBR(mbrResult* mbr, uint8_t* src);
            Z80SPI z80spi;             
            uint8_t sdCmdRxBuffer[SD_COMMAND_BUFFER_LENGTH];
    
    };

#endif