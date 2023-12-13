#include <FlashLoader.h>
#include <HexRecord.h>
#include <Z80TestProgram.h>

/* Types and definitions -------------------------------------------------------------------------------- */  
#define BYTE_READ_SETUP_TIME_us     10
#define BYTE_READ_PULSE_TIME_us     20
#define BYTE_READ_HOLD_TIME_us      20

#define BYTE_WRITE_SETUP_TIME_us    20
#define BYTE_WRITE_PULSE_TIME_us    20
#define BYTE_WRITE_HOLD_TIME_us     80

#define HEX_TYPE_COMMUNICATION      0xAA
#define HEX_TYPE_DATA               0x00
#define HEX_TYPE_END_OF_FILE        0x01

#define HEX_MESSAGE_ERROR_0         0xE0
#define HEX_MESSAGE_ERROR_1         0xE1
#define HEX_MESSAGE_ACKNOWLEDGE_0   0xA0
#define HEX_MESSAGE_ACKNOWLEDGE_1   0xA1
#define HEX_MESSAGE_FUNCTION_0      0xF0

uint8_t txBuffer[HEX_RECORD_MAX_STRING_LEN];
HexRecord rxHex, txHex;

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

    if ((flashmode == active) && (timer != 0)) {
        if (millis() - timer > FLASHLOADER_MAX_WAITIME_s * 1000) setFlashMode(false);
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
                bool commandOK = true;

                //if it is the first record received erase flash                
                if (hexCounter == 0) eraseFlash();
                hexCounter++;
                //reset timer
                timer = millis();

                //write data to flash
                for (int i=0; i<rxHex.payloadLength; i++) writeByte(rxHex.address + i, rxHex.payload[i]);

                //verify data
                for (int i=0; i<rxHex.payloadLength; i++) if(readByte(rxHex.address + i) != rxHex.payload[i]) commandOK = false;

                //send response
                uint8_t message = HEX_MESSAGE_ACKNOWLEDGE_1;
                if (!commandOK) message = HEX_MESSAGE_ERROR_1;
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
                delay(10);
                z80bus.resetZ80();
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

    z80bus.write_controlBit(z80bus.mreq, false);
    z80bus.write_addressBus(address);
    delayMicroseconds(BYTE_READ_SETUP_TIME_us);
    z80bus.write_controlBit(z80bus.rd, false);
    delayMicroseconds(BYTE_READ_PULSE_TIME_us);
    uint8_t data = z80bus.read_dataBus();
    z80bus.write_controlBit(z80bus.rd, true);
    delayMicroseconds(BYTE_READ_HOLD_TIME_us);

    z80bus.write_controlBit(z80bus.mreq, true);
    z80bus.release_addressBus();
    delayMicroseconds(BYTE_READ_HOLD_TIME_us);

    return data;
}

/*--------------------------------------------------------------------------------------------------------
 single one byte write access to flash (without setting mreq, this is handled by the overlying function)
---------------------------------------------------------------------------------------------------------*/
void FlashLoader::singleByteWrite(uint16_t address, uint8_t data) {
    z80bus.write_addressBus(address);
    z80bus.write_dataBus(data);
    delayMicroseconds(BYTE_WRITE_SETUP_TIME_us);
    z80bus.write_controlBit(z80bus.wr, false);
    delayMicroseconds(BYTE_WRITE_PULSE_TIME_us);
    z80bus.write_controlBit(z80bus.wr, true);
    delayMicroseconds(BYTE_WRITE_HOLD_TIME_us);
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
    z80bus.release_addressBus();
    z80bus.release_dataBus();
    delayMicroseconds(BYTE_WRITE_HOLD_TIME_us);
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
    z80bus.release_addressBus();
    z80bus.release_dataBus();
    delayMicroseconds(BYTE_WRITE_HOLD_TIME_us);

    delay(100);
}


/*--------------------------------------------------------------------------------------------------------
 to start and stop flash mode
---------------------------------------------------------------------------------------------------------*/
void FlashLoader::setFlashMode(bool enable_nDisable) {
    if (enable_nDisable) {    
        z80bus.request_bus();        
        flashmode = active;
        hexCounter = 0;
        rxHex.rxReset();        
        timer = millis();        
    }
    else {
        z80bus.release_bus();
        flashmode = inactive;
    }
}