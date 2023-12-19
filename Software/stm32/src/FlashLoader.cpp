#include <FlashLoader.h>
#include <Z80TestPrograms.h>

/* Types and definitions -------------------------------------------------------------------------------- */  
// Timeout for automatic aborting flash mode
#define FLASHLOADER_TIMEOUT_s       10

//flash timings
#define BYTE_WRITE_WAIT_TIME_us      25
#define FLASH_ERASE_WIAT_TIME_ms    100
#define BUS_STABILIZATION_TIME_us     1
#define FLASH_ACCESS_PULSE_TIME_us    1
#define FLASH_ACCESS_HOLD_TIME_us     1
#define FLASH_IDMODE_ACCESS_TIME_us  10

//Communication constants
#define HEX_TYPE_COMMUNICATION      0xAA
#define HEX_TYPE_DATA               0x00
#define HEX_TYPE_END_OF_FILE        0x01

#define HEX_MESSAGE_ERROR_0         0xE0
#define HEX_MESSAGE_ERROR_1         0xE1
#define HEX_MESSAGE_ACKNOWLEDGE_0   0xA0
#define HEX_MESSAGE_ACKNOWLEDGE_1   0xA1
#define HEX_MESSAGE_FUNCTION_0      0xF0

/*--------------------------------------------------------------------------------------------------------
 Constructor
---------------------------------------------------------------------------------------------------------*/
FlashLoader::FlashLoader(Z80bus bus) : z80bus(bus) {
    flashmode = inactive;
    chipVendorId = 0;
    chipDeviceId = 0;
}

/*--------------------------------------------------------------------------------------------------------
 process, run in main loop 
---------------------------------------------------------------------------------------------------------*/
void FlashLoader::process(void) {

    if ((flashmode == active) && (timer != 0)) {
        if (millis() - timer > FLASHLOADER_TIMEOUT_s * 1000) setFlashMode(false);
    }

}

/*--------------------------------------------------------------------------------------------------------
 reception of new serial characters
---------------------------------------------------------------------------------------------------------*/
void FlashLoader::serialUpdate(uint8_t c) {
    
    //upüdate hex recpetion and check result
    switch (rxHex.rxUpdate(c)) {
 
        case rxHex.incomplete: {
            break;
        }

        case rxHex.error: {
            //send back error message
            uint8_t message = HEX_MESSAGE_ERROR_0;
            txHex.data(HEX_TYPE_COMMUNICATION, 0x0000, &message, 1);
            uint8_t txLen = txHex.getString(txBuffer);
            Serial.write(txBuffer, txLen);
            break;
        }

        case rxHex.valid: {
            //check message            
            if ((rxHex.type == 0xAA) && (rxHex.payload[0] == HEX_MESSAGE_FUNCTION_0)) {
                //welcome message received
                //send back Acknowledge 1
                uint8_t message = HEX_MESSAGE_ACKNOWLEDGE_1;
                txHex.data(HEX_TYPE_COMMUNICATION, 0x0000, &message, 1);
                uint8_t txLen = txHex.getString(txBuffer);
                Serial.write(txBuffer, txLen);
            }
            //data message
            if (rxHex.type == HEX_TYPE_DATA) {
                //reset timer
                timer = millis();

                //if it is the first record received reset the board (to make sure flash is visible), and erase flash                
                if (hexCounter == 0) eraseFlash();
                hexCounter++;  

                bool writeOK = true;                         
                //write data to flash
                for (int i=0; i<rxHex.payloadLength; i++) writeByte(rxHex.address + i, rxHex.payload[i]);                    
                //verify data
                for (int i=0; i<rxHex.payloadLength; i++) if(readByte(rxHex.address + i) != rxHex.payload[i]) writeOK = false;

                //send response
                uint8_t message = HEX_MESSAGE_ACKNOWLEDGE_1;
                if (!writeOK) message = HEX_MESSAGE_ERROR_1;
                txHex.data(HEX_TYPE_COMMUNICATION, 0x0000, &message, 1);
                uint8_t txLen = txHex.getString(txBuffer);
                Serial.write(txBuffer, txLen);
            }
            
            if (rxHex.type == HEX_TYPE_END_OF_FILE) {
                //end of file record received, we are done
                //send back acknowledge, then stop flashmode and give control to Z80
                uint8_t message = HEX_MESSAGE_ACKNOWLEDGE_1;
                txHex.data(HEX_TYPE_COMMUNICATION, 0x0000, &message, 1);
                uint8_t txLen = txHex.getString(txBuffer);
                Serial.write(txBuffer, txLen);
                
                //stop flashmode, delay, and reset the Z80
                setFlashMode(false);                
            }

            break;
        }

    }
}

/*--------------------------------------------------------------------------------------------------------
 read a single byte from the flash - works in active mode only
---------------------------------------------------------------------------------------------------------*/
uint8_t FlashLoader::readByte(uint16_t address) {
    if (flashmode != active) return 0xFF;

    z80bus.write_addressBus(address);
    delayMicroseconds(BUS_STABILIZATION_TIME_us);
    z80bus.write_controlBit(z80bus.mreq, false);
    z80bus.write_controlBit(z80bus.rd, false);
    delayMicroseconds(FLASH_ACCESS_PULSE_TIME_us);
    uint8_t data = z80bus.read_dataBus();
    z80bus.write_controlBit(z80bus.rd, true);
    z80bus.write_controlBit(z80bus.mreq, true);
    delayMicroseconds(FLASH_ACCESS_HOLD_TIME_us);
    
    return data;
}

/*--------------------------------------------------------------------------------------------------------
 single one byte write access to flash (without setting mreq, this is handled by the overlying function)
---------------------------------------------------------------------------------------------------------*/
void FlashLoader::singleByteWrite(uint16_t address, uint8_t data) {
    z80bus.write_dataBus(data);
    z80bus.write_addressBus(address);
    delayMicroseconds(BUS_STABILIZATION_TIME_us);
    z80bus.write_controlBit(z80bus.wr, false);
    delayMicroseconds(FLASH_ACCESS_PULSE_TIME_us);
    z80bus.write_controlBit(z80bus.wr, true);
    delayMicroseconds(FLASH_ACCESS_HOLD_TIME_us);
}

/*--------------------------------------------------------------------------------------------------------
 write a single byte from the flash - works in active mode only
---------------------------------------------------------------------------------------------------------*/
void FlashLoader::writeByte(uint16_t address, uint8_t data) {
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
 flash erase - works in active mode only
---------------------------------------------------------------------------------------------------------*/
void FlashLoader::eraseFlash() {
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

    delay(FLASH_ERASE_WIAT_TIME_ms);
}

/*--------------------------------------------------------------------------------------------------------
 Read Vendor and Chip ID from device - works in active mode only
---------------------------------------------------------------------------------------------------------*/
void FlashLoader::readChipIndentification() {
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
    delayMicroseconds(BUS_STABILIZATION_TIME_us);
    z80bus.write_controlBit(z80bus.rd, false);
    z80bus.write_addressBus(0x0000);    
    delayMicroseconds(FLASH_ACCESS_PULSE_TIME_us);
    chipVendorId = z80bus.read_dataBus();
    z80bus.write_addressBus(0x0001);
    delayMicroseconds(FLASH_ACCESS_PULSE_TIME_us);
    chipDeviceId = z80bus.read_dataBus();
    z80bus.write_controlBit(z80bus.rd, true);
    delayMicroseconds(FLASH_ACCESS_HOLD_TIME_us);

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
uint32_t FlashLoader::bytesProgrammed() {
    if (flashmode != active) return 0;
    for (uint32_t i=0; i<0xFFFF; i++) if(readByte(0xFFFF - i) != 0xFF) return 0xFFFF - i + 1;
    return 0;
}

/*--------------------------------------------------------------------------------------------------------
 Writes and verifies a test program to the FLASH
 works in active mode only
---------------------------------------------------------------------------------------------------------*/
bool FlashLoader::writeTestProgram(uint8_t programNumber) {
    setFlashMode(true);

    //erase
    eraseFlash();    
    //write
    for (uint32_t i=0; i<testPrograms[programNumber].length; i++) writeByte(i, testPrograms[programNumber].data[i]);
    //verify
    bool programOK = true;
    for (uint32_t i=0; i<testPrograms[programNumber].length; i++) if (readByte(i) != testPrograms[programNumber].data[i]) programOK = false;
    
    setFlashMode(false);                 
    return programOK;
}

/*--------------------------------------------------------------------------------------------------------
 to start and stop flash mode
---------------------------------------------------------------------------------------------------------*/
void FlashLoader::setFlashMode(bool enable_nDisable) {
    if (enable_nDisable) {    
        z80bus.request_bus();     
        z80bus.write_controlBit(z80bus.reset, true);   
        flashmode = active;
        hexCounter = 0;
        rxHex.rxReset();        
        timer = millis();        
    }
    else {
        z80bus.release_bus();
        z80bus.write_controlBit(z80bus.reset, false); 
        flashmode = inactive;
    }
}