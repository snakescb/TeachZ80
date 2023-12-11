#include <FlashLoader.h>
#include <Z80TestProgram.h>

/* Types and definitions -------------------------------------------------------------------------------- */  
#define BYTE_READ_SETUP_TIME_us     50
#define BYTE_READ_PULSE_TIME_us     100
#define BYTE_READ_HOLD_TIME_us      100

#define BYTE_WRITE_SETUP_TIME_us    50
#define BYTE_WRITE_PULSE_TIME_us    100
#define BYTE_WRITE_HOLD_TIME_us     100

/*--------------------------------------------------------------------------------------------------------
 Constructor
---------------------------------------------------------------------------------------------------------*/
FlashLoader::FlashLoader(Z80bus bus) : z80bus(bus) {
    flashmode = inactive;
}

/*--------------------------------------------------------------------------------------------------------
 process, run in main loop 
---------------------------------------------------------------------------------------------------------*/
void FlashLoader::process(void) {
    
}

/*--------------------------------------------------------------------------------------------------------
 read a single byte from the flash - works in active mode only
---------------------------------------------------------------------------------------------------------*/
uint8_t FlashLoader::readByte(uint16_t address) {
    if (flashmode != active) return 0xFF;

    z80bus.write_controlBit(z80bus.mreq, false);
    z80bus.write_addressBus(address);
    flashDelay(BYTE_READ_SETUP_TIME_us);
    z80bus.write_controlBit(z80bus.rd, false);
    flashDelay(BYTE_READ_PULSE_TIME_us);
    uint8_t data = z80bus.read_dataBus();
    z80bus.write_controlBit(z80bus.rd, true);
    flashDelay(BYTE_READ_HOLD_TIME_us);

    z80bus.write_controlBit(z80bus.mreq, true);
    z80bus.release_addressBus();
    flashDelay(BYTE_READ_HOLD_TIME_us);

    return data;
}

/*--------------------------------------------------------------------------------------------------------
 single one byte write access to flash (without setting mreq, this is handles by the overlying function)
---------------------------------------------------------------------------------------------------------*/
void FlashLoader::singleWrite(uint16_t address, uint8_t data) {
    z80bus.write_addressBus(address);
    z80bus.write_dataBus(data);
    flashDelay(BYTE_WRITE_SETUP_TIME_us);
    z80bus.write_controlBit(z80bus.wr, false);
    flashDelay(BYTE_WRITE_PULSE_TIME_us);
    z80bus.write_controlBit(z80bus.wr, true);
    flashDelay(BYTE_WRITE_HOLD_TIME_us);
}

/*--------------------------------------------------------------------------------------------------------
 write a single byte from the flash - works in active mode only
---------------------------------------------------------------------------------------------------------*/
void FlashLoader::writeByte(uint16_t address, uint8_t data) {
    if (flashmode != active) return;

    //enable chip
    z80bus.write_controlBit(z80bus.mreq, false);

    //load address and data, 3-byte load unlock for SST39SF0x0
    singleWrite(0x5555, 0xAA);
    singleWrite(0x2AAA, 0x55);
    singleWrite(0x5555, 0xA0);

    //Actual byte programming
    singleWrite(address, data);
    
    //disable chip and release bus
    z80bus.write_controlBit(z80bus.mreq, true);
    z80bus.release_addressBus();
    z80bus.release_dataBus();
    flashDelay(BYTE_WRITE_HOLD_TIME_us);
}

/*--------------------------------------------------------------------------------------------------------
 flash erase - works in active mode only
---------------------------------------------------------------------------------------------------------*/
void FlashLoader::eraseFlash() {
    if (flashmode != active) return;

    //enable chip
    z80bus.write_controlBit(z80bus.mreq, false);

    //erase, 6-byte load unlock for SST39SF0x0
    singleWrite(0x5555, 0xAA);
    singleWrite(0x2AAA, 0x55);
    singleWrite(0x5555, 0x80);
    singleWrite(0x5555, 0xAA);
    singleWrite(0x2AAA, 0x55);
    singleWrite(0x5555, 0x10);

    //disable chip and release bus
    z80bus.write_controlBit(z80bus.mreq, true);
    z80bus.release_addressBus();
    z80bus.release_dataBus();
    flashDelay(BYTE_WRITE_HOLD_TIME_us);

    delay(100);
    Serial.println("Flash: Chip erased");
}


/*--------------------------------------------------------------------------------------------------------
 to start and stop flash mode
---------------------------------------------------------------------------------------------------------*/
void FlashLoader::setFlashMode(bool enable_nDisable) {
    if (enable_nDisable) {    
        z80bus.release_bus();
        z80bus.request_bus();
        flashmode = active;
        flashIsErased = false;


        //flash testprogram
        z80bus.write_controlBit(z80bus.reset, false);
        Serial.println("Flash: Loading Testprogram");
        eraseFlash();
        for (int i=0; i<Z80_TEST_PROGRAM_LENGTH; i++) {
            writeByte(i, z80TestProgram[i]);
            Serial.printf("Address: %04X, Value: %02X\r\n", i, readByte(i));
        }
        Serial.println("Done");
        z80bus.release_bus();
        z80bus.write_controlBit(z80bus.reset, true);

    }
    else {
        z80bus.release_bus();
        flashmode = inactive;
    }
}

/*--------------------------------------------------------------------------------------------------------
 simple delay method burning some microseconds
---------------------------------------------------------------------------------------------------------*/
void FlashLoader::flashDelay(uint16_t count) {
    delayMicroseconds(count);
}