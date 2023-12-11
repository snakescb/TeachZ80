/* -------------------------------------------------------------------------------------------------------
 Library for bruning the TeachZ80 flash 

 This library is not general purpose, it coded to work together with the TeachZ80 Z80bus library.
 This code was developed specifically for the SST39SF0x0 Flash chips. It probably works for other chips as well
 but it may be adopted. The byte/address combination coded here for flash erasing and programming may be specific for 
 SST39SF0x0 flash memories.

 This code has been specifically tested with the SST39SF010 flash chips
 Typical flash programming operation:
    1. Pull Z80 Board reset line low. This is required to ensure the flash receives the mreq signal through the shadow flash logic
    2. Chip identification
    3. Erasing the whole flash
    4. Byte-by-Byte programming
    5. Byte-by-Byte verification
    6. Release Z80 reset line

 Flashing always must be enabled by the mode button. When flash mode is entered, simple flash command
 packets in Intel HEX format can be sent through the serial port. 
 See here: https://en.wikipedia.org/wiki/Intel_HEX
 Supported is the HEX-80 recordset, so record types 00 (data) and 01 (end of file)
 Record type 0xB0 is used to send confirmation data back to the host after every received command

 When a data record is received:
    - If the record is not valid (eg wrong checksum) a hex record 0xB0 to address 0x00 and 1 data byte is sent, 0xE0 (Error 0)
    - If the record is accepted
        - If it is the first record after entering the flash mode, step 1, 2 and 3 are performed
        - The received bytes are stored in the flash chip starting with the provided address
        - when completed, the written bytes are verified
        - If the verification is correct, a hex record 0xB0 to address 0x00 and 1 data byte is sent, 0xA0 (Acknowledge 0) 
        - If the verification is incorrect, a hex record 0xB0 to address 0x00 and 1 data byte is sent, 0xE1 (Error 1) 

 When a end of file record is received
    - The Z80 address, data and control bus is released
    - The Z80 reset line is released
    - flash mode is exited
    - A hex record 0xB0 to address 0x00 and 1 data byte is sent, 0xA1 (Acknowledge 1) 
 
 Author: Christian Luethi
--------------------------------------------------------------------------------------------------------- */

#ifndef FLASHLOADER_H
#define FLASHLOADER_H

    #include <Arduino.h>
    #include <Z80bus.h>    

    class FlashLoader {

        enum flashMode: uint8_t { active, inactive }; 
        flashMode flashmode;

        public:
            FlashLoader(Z80bus bus);    
            void process(void); 
            void serialUpdate(uint8_t c); 
            void setFlashMode(bool enable_nDisable = true);

        private:
            Z80bus z80bus;
            bool flashIsErased;
            void flashDelay(uint16_t count);
            void readChipIndentification();
            uint8_t readByte(uint16_t address);            
            void writeByte(uint16_t address, uint8_t data);
            void singleWrite(uint16_t address, uint8_t data);
            void eraseFlash();

    };

#endif