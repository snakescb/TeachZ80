/* -------------------------------------------------------------------------------------------------------
 Library for bruning the TeachZ80 flash 

 This library is not general purpose, it coded to work together with the TeachZ80 Z80bus library.
 This code was developed specifically for the SST39SF0x0 Flash chips. It probably works for other chips as well
 but it may be adopted. The byte/address combination coded here for flash erasing and programming may be specific for 
 SST39SF0x0 flash memories.

 This code has been specifically tested with the SST39SF010 flash chips
 Typical flash programming operation:
    1. Pull Z80 Board reset line low, keep nIOREQ high. This is required to ensure the flash receives the mreq signal through the shadow flash logic
    2. Chip identification
    3. Erasing the whole flash
    4. Byte-by-Byte programming
    5. Byte-by-Byte verification
    6. Release Z80 reset line

 Flashing always must be enabled by the mode button. When flash mode is entered, simple flash command
 packets can be sent through the serial port in the following format:
    Write Package: { 0x01, AddrH, AddrL, Byte 0, Byte 1, ... , Byte N-1, Checksum }
    Read  Package: { 0x02, AddrH, AddrL, NumBytes, Checksum }
    End Package:   { 0x03, 0xFC }

 {, } and \ characters are control characters to identify the start and stop of a package and to byte pad subsequent contgrol bytes
 If {, }, or \ characters are supposed to be interpreted as data bytes, they need to be padded with a preceeding \ character
 Checksum is an inverted XOR of all bytes sent starting with the byte followed by { and stopping with second-last byte before }
 The amount of bytes provided or requested per package must be <= 128

 When a complete Write packet is received:
    - If the checksum is wrong, { 0xBF, 0x40 } is returned (General Error package)
    - If the checksum is correct
        - If it is the first write package after entering the flash mode, step 1, 2 and 3 are performed
        - The received bytes are stored in the flash chip starting with the provided address
        - when completed, { 0xB0, 0x4F } is returned (General Ok package)

 When a complete Read packet is received:
    - If the checksum is wrong, { 0xBF, 0x40 } is returned (General Error package)
    - If the checksum is correct
        - The requested amount of bytes starting with the address is read from the flash
        - { 0xB1, AddrH, AddrL, Byte 0, Byte 1, ... , Byte N-1, Checksum } is returned

 When a complete End Package is received
    - The Z80 address, data and control bus is released
    - The Z80 reset line is released
    - flash mode is exited
 
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
            void flashDelay(uint16_t count);
            void readChipIndentification();
            uint8_t readByte(uint16_t address);

    };

#endif