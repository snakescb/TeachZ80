#include <Z80Flash.h>
#include <z80Programs.h>

/* Types and definitions -------------------------------------------------------------------------------- */  
//flash timings
#define BYTE_WRITE_WAIT_TIME_us      25
#define PAGE_ERASE_WAIT_TIME_ms      25
#define FLASH_ERASE_WAIT_TIME_ms    100
#define FLASH_IDMODE_ACCESS_TIME_us  10

/*--------------------------------------------------------------------------------------------------------
 Constructor
---------------------------------------------------------------------------------------------------------*/
Z80Flash::Z80Flash(Z80Bus bus) : z80bus(bus) {
    flashmode = inactive;
    chipVendorId = 0;
    chipDeviceId = 0;
}

/*--------------------------------------------------------------------------------------------------------
 read a single byte from the flash - works in active mode only
---------------------------------------------------------------------------------------------------------*/
uint8_t Z80Flash::readByte(uint16_t address) {
    if (flashmode != active) return 0xFF;
    z80bus.write_addressBus(address);
    z80bus.write_controlBit(z80bus.mreq, false);
    z80bus.write_controlBit(z80bus.rd, false);
    uint8_t data = z80bus.read_dataBus();
    z80bus.write_controlBit(z80bus.rd, true);
    z80bus.write_controlBit(z80bus.mreq, true);
    
    return data;
}

/*--------------------------------------------------------------------------------------------------------
 single one byte write access to flash (without setting mreq, this is handled by the overlying function)
---------------------------------------------------------------------------------------------------------*/
void Z80Flash::singleByteWrite(uint16_t address, uint8_t data) {
    z80bus.write_dataBus(data);
    z80bus.write_addressBus(address);
    z80bus.write_controlBit(z80bus.wr, false);
    z80bus.write_controlBit(z80bus.wr, true);
}

/*--------------------------------------------------------------------------------------------------------
 write a single byte from the flash - works in active mode only
---------------------------------------------------------------------------------------------------------*/
void Z80Flash::writeByte(uint16_t address, uint8_t data) {
    if (flashmode != active) return;

    //enable chip
    z80bus.write_controlBit(z80bus.mreq, false);

    //load address and data, 3-byte program command for SST39SF0x0
    singleByteWrite(0x5555, 0xAA);
    singleByteWrite(0x2AAA, 0x55);
    singleByteWrite(0x5555, 0xA0);

    //Programming the actual data byte
    singleByteWrite(address, data);
    
    //disable chip and release bus
    z80bus.write_controlBit(z80bus.mreq, true);    
    z80bus.release_dataBus();

    delayMicroseconds(BYTE_WRITE_WAIT_TIME_us);
}

/*--------------------------------------------------------------------------------------------------------
 flash erase - erases the whole chip - works in active mode only
---------------------------------------------------------------------------------------------------------*/
void Z80Flash::eraseFlash() {
    if (flashmode != active) return;

    //enable chip
    z80bus.write_controlBit(z80bus.mreq, false);

    //erase, 6-byte erase command for SST39SF0x0
    singleByteWrite(0x5555, 0xAA);
    singleByteWrite(0x2AAA, 0x55);
    singleByteWrite(0x5555, 0x80);
    singleByteWrite(0x5555, 0xAA);
    singleByteWrite(0x2AAA, 0x55);
    singleByteWrite(0x5555, 0x10);   

    //disable chip and release bus
    z80bus.write_controlBit(z80bus.mreq, true);
    z80bus.release_dataBus();

    delay(FLASH_ERASE_WAIT_TIME_ms);
}

/*--------------------------------------------------------------------------------------------------------
 bank erase - erases the currently selected 64k bank - works in active mode only
---------------------------------------------------------------------------------------------------------*/
void Z80Flash::eraseBank() {
    if (flashmode != active) return;

    //enable chip
    z80bus.write_controlBit(z80bus.mreq, false);

    for (int i=0; i<16; i++) {
        //erase, 6-byte erase command for SST39SF0x0
        singleByteWrite(0x5555, 0xAA);
        singleByteWrite(0x2AAA, 0x55);
        singleByteWrite(0x5555, 0x80);
        singleByteWrite(0x5555, 0xAA);
        singleByteWrite(0x2AAA, 0x55);
        singleByteWrite(i << 12, 0x30);
        delay(PAGE_ERASE_WAIT_TIME_ms);
    }   

    //disable chip and release bus
    z80bus.write_controlBit(z80bus.mreq, true);
    z80bus.release_dataBus();

    delay(FLASH_ERASE_WAIT_TIME_ms);
}

/*--------------------------------------------------------------------------------------------------------
 Read Vendor and Chip ID from device - works in active mode only
---------------------------------------------------------------------------------------------------------*/
void Z80Flash::readChipIndentification() {
    if (flashmode != active) return;

    //enable chip
    z80bus.write_controlBit(z80bus.mreq, false);    

    //enter device identification mode
    singleByteWrite(0x5555, 0xAA);
    singleByteWrite(0x2AAA, 0x55);
    singleByteWrite(0x5555, 0x90);
    delayMicroseconds(FLASH_IDMODE_ACCESS_TIME_us);

    //read ID's
    z80bus.release_dataBus();
    z80bus.write_controlBit(z80bus.rd, false);
    z80bus.write_addressBus(0x0000);    
    delayMicroseconds(FLASH_IDMODE_ACCESS_TIME_us);
    chipVendorId = z80bus.read_dataBus();
    z80bus.write_addressBus(0x0001);
    delayMicroseconds(FLASH_IDMODE_ACCESS_TIME_us);
    chipDeviceId = z80bus.read_dataBus();
    z80bus.write_controlBit(z80bus.rd, true);

    //exit device id mode
    singleByteWrite(0x5555, 0xAA);
    singleByteWrite(0x2AAA, 0x55);
    singleByteWrite(0x5555, 0xF0);
    delayMicroseconds(FLASH_IDMODE_ACCESS_TIME_us);

    z80bus.write_controlBit(z80bus.mreq, true);
    z80bus.release_dataBus();
}

/*--------------------------------------------------------------------------------------------------------
 Checks how many bytes are programmed in the flash (bottom 64k)
 reads the flash starting from the highest address, until a byte does not read FF anymore
 works in active mode only
---------------------------------------------------------------------------------------------------------*/
uint32_t Z80Flash::bytesProgrammed() {
    if (flashmode != active) return 0;
    for (uint32_t i=0; i<0xFFFF; i++) if(readByte(0xFFFF - i) != 0xFF) return 0xFFFF - i + 1;
    return 0;
}

/*--------------------------------------------------------------------------------------------------------
 Writes and verifies a test program to the FLASH
 works in active mode only
---------------------------------------------------------------------------------------------------------*/
bool Z80Flash::writeProgram(uint8_t programNumber) {
    setMode(true);

    //erase current bank, write, verify
    eraseBank();    
    for (uint32_t i=0; i<z80FlashPrograms[programNumber].length; i++) writeByte(i, z80FlashPrograms[programNumber].data[i]);
    bool programOK = true;
    for (uint32_t i=0; i<z80FlashPrograms[programNumber].length; i++) if (readByte(i) != z80FlashPrograms[programNumber].data[i]) programOK = false;

    setMode(false);                 
    return programOK;
}

/*--------------------------------------------------------------------------------------------------------
 to start and stop flash mode
---------------------------------------------------------------------------------------------------------*/
void Z80Flash::setMode(bool modeactive) {
    if (modeactive) {    
        z80bus.request_bus();     
        z80bus.write_controlBit(z80bus.reset, true);   
        flashmode = active;      
    }
    else {
        z80bus.release_bus();
        z80bus.write_controlBit(z80bus.reset, false); 
        flashmode = inactive;
    }
}