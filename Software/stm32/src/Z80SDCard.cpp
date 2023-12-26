#include <Z80SDCard.h>

/* Types and definitions -------------------------------------------------------------------------------- */  
#define SPI_OUT_IOPORT      0x10
#define SPI_IN_IOPORT       0x00
#define SPI_OUT_MOSI        0x01
#define SPI_OUT_CLK         0x02
#define SPI_OUT_SSEL        0x04
#define SPI_IN_MISO         0x80

const uint8_t cmd0[]   =  {  0 | 0x40, 0x00, 0x00, 0x00, 0x00, 0x94 | 0x01 };
const uint8_t cmd8[]   =  {  8 | 0x40, 0x00, 0x00, 0x01, 0xAA, 0x86 | 0x01 };
const uint8_t cmd55[]  =  { 55 | 0x40, 0x00, 0x00, 0x01, 0xAA, 0x86 | 0x01 };
const uint8_t acmd41[] =  { 41 | 0x40, 0x40, 0x00, 0x00, 0x00, 0x00 | 0x01 };
const uint8_t cmd58[]  =  { 58 | 0x40, 0x40, 0x00, 0x00, 0x00, 0x00 | 0x01 };

/*--------------------------------------------------------------------------------------------------------
 Constructor
---------------------------------------------------------------------------------------------------------*/
Z80SDCard::Z80SDCard(Z80SPI spi) : z80spi(spi) {
    
}

/*--------------------------------------------------------------------------------------------------------
 Start accessing the card 
    - request access to the Z80 bus
    - check if a card is present in the slot
    - idle the SPI lines
    - wakeup the card by sneding 80 clocks with ssel high
    - send CMD0 (GO_IDLE_STATE), check R1 response (must be 0x01)
    - send CMD8 (SEND_IF_CONDITION), check R7 rsponse (must be 0x01, 0x00, 0x00, 0x01, 0xAA)
    - send CMD55 (APP_CMD), check R1 response (must be 0x01), directly followed by 
    - send ACMD41 (APP_CMD_41), check R1 == 0 (card is ready). repeat last two steps for a maximum amount of tries with some delay
    - send CMD58 (READ_OCR), check if its SDHC or SDXC card (bit 6 in 2nd response byte is set). Only these cards are supported, they have 512 bytes blockside by default
 
 Stop again when done to release bus
 Z80 may be reset when completed, SPI changes the output buffers uncontrolled. It's up to the context
---------------------------------------------------------------------------------------------------------*/
Z80SDCard::sdResult Z80SDCard::accessCard(bool state) {
    if (state) {
        //request bus
        z80spi.requestBus(true);
        //check if card is present
        if(z80spi.checkSDDetect()) return nocard;
        //wake up card
        wakeupCard();
        //CMD0
        sdCommand((uint8_t*)cmd0, 6, 1);
        if (sdCmdRxBuffer[0] != 0x01) return not_idle;
        //CMD8
        sdCommand((uint8_t*)cmd8, 6, 5);
        if (sdCmdRxBuffer[0] != 0x01) return invalid_status;
        //Loop of CMD55 followed by ACMD41  
        //according spec can take up to one second      
        bool cardReady = false;
        for (uint16_t i=0; i<1000; i++) {
            sdCommand((uint8_t*)cmd55, 6, 1);
            if (sdCmdRxBuffer[0] == 0x01) {
                sdCommand((uint8_t*)acmd41, 6, 1);
                if (sdCmdRxBuffer[0] == 0x00) {
                    cardReady = true;
                    break;
                }
            }
            delayMicroseconds(1000);
        }
        if (!cardReady) return not_ready;
        //CMD58
        sdCommand((uint8_t*)cmd58, 6, 5);  
        if (sdCmdRxBuffer[0] != 0x00) return invalid_capacity;
        if (!(sdCmdRxBuffer[1] & 0x40)) return invalid_capacity;
        //done, we know we have a spec 2 card, hc or xd, with 512 byte blocks, and it's ready to read and write data
    }
    else {
        z80spi.requestBus(false);
    }
    return ok;
}

/*--------------------------------------------------------------------------------------------------------
 Read one block of data from the SD. Assumes card was accessed before successfully
    - generates CMD17 with the block number (big endian)
    - send the command to the card, and check if the command is accepted
    - read the card, until it has its data ready
    - read the block
    - read 2 bytes of crc (no CRC checking implemented tough)
---------------------------------------------------------------------------------------------------------*/
Z80SDCard::sdResult Z80SDCard::readBlock(uint32_t blockNumber, uint8_t* dst) {
    sdCmdTxBuffer[0] = 17 | 0x40;
    sdCmdTxBuffer[1] = blockNumber >> 24;
    sdCmdTxBuffer[2] = blockNumber >> 16;
    sdCmdTxBuffer[3] = blockNumber >>  8;
    sdCmdTxBuffer[4] = blockNumber;
    sdCmdTxBuffer[5] = 0 | 0x01;

    //control the ssel manually, not via sdCommand
    selectCard(false);
    //send the command to the card
    sdCommand(sdCmdTxBuffer, 6, 1, 15, false);

    //ceck if the card accepted the command (is ready)
    if (sdCmdRxBuffer[0] != 0x00) { selectCard(true); return not_ready; }

    //wait for the card having the data ready
    bool cardReady = false;
    for (int i=0; i<1000; i++) {
        uint8_t rx = z80spi.readByte();
        if (rx != 0xFF) {
            cardReady = true;
            break;
        }
    }
    if (!cardReady) { selectCard(true); return read_timeout; }

    //read data
    for (int i=0; i<SD_BLOCK_SIZE; i++) dst[i] = z80spi.readByte();

    //read crc
    z80spi.readByte();
    z80spi.readByte();

    //done
    selectCard(true);
    return ok;
}

/*--------------------------------------------------------------------------------------------------------
 Write one block of data from the SD. Assumes card was accessed before successfully
    - generates CMD24 with the block number (big endian)
    - send the command to the card, and check if the command is accepted
    - generate 8 clocks
    - send start token, 0xFE
    - send data block
    - wait for completion status
    - check completion status
    - set ssel line
    - wait for complete message from card
---------------------------------------------------------------------------------------------------------*/
Z80SDCard::sdResult Z80SDCard::writeBlock(uint32_t blockNumber, uint8_t* src) {
    sdCmdTxBuffer[0] = 24 | 0x40;
    sdCmdTxBuffer[1] = blockNumber >> 24;
    sdCmdTxBuffer[2] = blockNumber >> 16;
    sdCmdTxBuffer[3] = blockNumber >>  8;
    sdCmdTxBuffer[4] = blockNumber;
    sdCmdTxBuffer[5] = 0 | 0x01;

    //control the ssel manually, not via sdCommand
    selectCard(false);
    //send the command to the card
    sdCommand(sdCmdTxBuffer, 6, 1, 15, false);

    //ceck if the card accepted the command (is ready)
    if (sdCmdRxBuffer[0] != 0x00) { selectCard(true); return not_ready; }

    //dummy write to generate clocks, then send start token
    z80spi.writeByte(0xFF);
    z80spi.writeByte(0xFE);

    //send data
    for (int i=0; i<SD_BLOCK_SIZE; i++) z80spi.writeByte(src[i]);

    //wait for completion status, can take 250ms
    bool cardComplete = false;
    uint8_t lastRx;
    for (int i=0; i<10000; i++) {
        lastRx = z80spi.readByte();
        if (lastRx != 0xFF) {
            cardComplete = true;
            break;
        }
    }
    if (!cardComplete) { selectCard(true); return write_timeout_1; }

    //check if everything was ok
    if ((lastRx & 0x1F) != 0x05) { selectCard(true); return write_error; }

    //set ssel line
    selectCard(true);

    //wait completion status
    cardComplete = false;
    for (int i=0; i<10000; i++) {
        lastRx = z80spi.readByte();
        if (lastRx == 0xFF) {
            cardComplete = true;
            break;
        }
    }
    if (!cardComplete) return write_timeout_2;    
    return ok;
}

/*--------------------------------------------------------------------------------------------------------
 Send command and wait for expected amount of bytes reponse (first accepted byte is byte with MSB = 0)
---------------------------------------------------------------------------------------------------------*/
void Z80SDCard::sdCommand(uint8_t* cmd, uint8_t txlen, uint8_t rxlen, uint8_t maxtries, bool controlssel) {
    //select the card
    if(controlssel) selectCard(false);
    //send the required amount of bytes
    for (int i=0; i<txlen; i++) z80spi.writeByte(cmd[i]);
    //check response. read until the byte has highest bit cleared. then read remaining bytes
    //put the data in the response buffer
    for (int i=0; i<maxtries; i++) {
        sdCmdRxBuffer[0] = z80spi.readByte();
        if (sdCmdRxBuffer[0] & 0x80 == 0) break;
    }
    for (int i=1; i<rxlen; i++) sdCmdRxBuffer[i] = z80spi.readByte();
    //deselect card
    if(controlssel) selectCard(true);
}

/*--------------------------------------------------------------------------------------------------------
 send 8 clocks, change the line, send 8 clocks again
---------------------------------------------------------------------------------------------------------*/
void Z80SDCard::selectCard(bool select) {
    z80spi.writeByte(0xFF);
    z80spi.slaveSelect(select);
    z80spi.writeByte(0xFF);
}

/*--------------------------------------------------------------------------------------------------------
 send 80 clocks, make sure SS is high, and delay 1 ms
---------------------------------------------------------------------------------------------------------*/
void Z80SDCard::wakeupCard() {
    z80spi.slaveSelect(true);
    delay(1);
    for (int i=0; i<10; i++) z80spi.writeByte(0xFF);
}
