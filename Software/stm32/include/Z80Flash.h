/* -------------------------------------------------------------------------------------------------------
 Library  TeachZ80 flash 

 This library it coded to work together with the TeachZ80 Z80bus library.
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
 
 Author: Christian Luethi
--------------------------------------------------------------------------------------------------------- */

#ifndef Z80_FLASH
#define Z80_FLASH

    #include <Arduino.h>
    #include <Z80bus.h> 
    #include <HexRecord.h>       

    class Z80Flash {
            
        public:
            enum Z80Flash_mode: uint8_t { active, inactive }; 
            Z80Flash_mode flashmode;
            uint8_t chipVendorId;
            uint8_t chipDeviceId;

            Z80Flash(Z80Bus bus);    
            void setMode(bool modeactive = true);
            uint8_t readByte(uint16_t address); 
            void writeByte(uint16_t address, uint8_t data);
            void eraseFlash();   
            void eraseBank();   
            uint32_t bytesProgrammed(void);
            bool writeProgram(uint8_t programNumber);
            void readChipIndentification();            

        private:            
            Z80Bus z80bus;      
            void singleByteWrite(uint16_t address, uint8_t data);         

    };

#endif