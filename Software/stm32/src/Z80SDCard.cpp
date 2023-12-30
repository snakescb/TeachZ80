#include <Z80SDCard.h>
#include <Z80Programs.h>

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
 Writes a program to the beginning of the selected partition
 ---------------------------------------------------------------------------------------------------------*/
Z80SDCard::sdResult Z80SDCard::writeProgram(uint8_t partition, uint8_t programNumber) {
    if (partition > 3) return invalid_partition;
    
    //read mbr and check if the requested partition is valid (size > 0)
    sdResult result = accessCard(true);
    if (result != ok) { accessCard(false); return result; }
    result = readBlock(0, sdDataBuffer);
    if (result != ok) { accessCard(false); return result; }
    mbrResult mbr;
    parseMBR(&mbr, sdDataBuffer);
    if (mbr.partitiontable[partition].size == 0) { accessCard(false); return invalid_partition; }

    //prepare the variable for writing the porgam
    uint32_t bytesToWrite = z80SDPrograms[programNumber].length;
    uint32_t blockNumber = mbr.partitiontable[partition].block;
    uint32_t byteIndex = 0;

    //write data
    while ((bytesToWrite > 0) && (result == ok)) {
        //prepare the data block to write
        //if blocked is not filled by the program, fill the rest with zeros
        uint16_t nextBlockSize = 512;
        if (nextBlockSize > bytesToWrite) nextBlockSize = bytesToWrite;
        bytesToWrite -= nextBlockSize;
        for (int i=0; i<nextBlockSize; i++) sdDataBuffer[i] = z80SDPrograms[programNumber].data[byteIndex++];
        for (int i=nextBlockSize; i<512; i++) sdDataBuffer[i] = 0;
        result = writeBlock(blockNumber++, sdDataBuffer);
    }

    accessCard(false);
    return result;
}

/*--------------------------------------------------------------------------------------------------------
 Formats the card and creates 4 partitions of 0x40000 block lengths, which is 128MBytes
 ---------------------------------------------------------------------------------------------------------*/
Z80SDCard::sdResult Z80SDCard::formatCard() {

    //initialize the block buffer
    for (int i=0; i<SD_BLOCK_SIZE; i++) sdDataBuffer[i] = 0;

    //sd partition data setup
    uint32_t partitionSize = 0x40000;
    uint32_t partitionStartBlock = 0x800;
    uint8_t partitionType = 0x83;
    uint8_t partitionStatus = 0x00;
    uint16_t baseAddress = 0x01BE;

    //create partitions
    for (int i=0; i<4; i++) {
        sdDataBuffer[baseAddress + 11] = partitionStartBlock >> 24;
        sdDataBuffer[baseAddress + 10] = partitionStartBlock >> 16;
        sdDataBuffer[baseAddress +  9] = partitionStartBlock >> 8;
        sdDataBuffer[baseAddress +  8] = partitionStartBlock;
        sdDataBuffer[baseAddress + 15] = partitionSize >> 24;
        sdDataBuffer[baseAddress + 14] = partitionSize >> 16;
        sdDataBuffer[baseAddress + 13] = partitionSize >> 8;
        sdDataBuffer[baseAddress + 12] = partitionSize;
        sdDataBuffer[baseAddress +  0] = partitionStatus;
        sdDataBuffer[baseAddress +  4] = partitionType;

        baseAddress += 16;
        partitionStartBlock += partitionSize;
    }
    sdDataBuffer[510] = 0x55;
    sdDataBuffer[511] = 0xAA;

    //access the card
    sdResult result = accessCard(true);
    if (result != ok) { accessCard(false); return result; }

    //write block 0
    result = writeBlock(0, sdDataBuffer);
    accessCard(false);
    return result;
}

/*--------------------------------------------------------------------------------------------------------
 parse the partition information of a given 512 byte block of data into an MBR structure
 ---------------------------------------------------------------------------------------------------------*/
void Z80SDCard::parseMBR(mbrResult* mbr, uint8_t* src) {

    //prepare result structure
    mbr->partitions = 0;
    for (int i=0; i<4; i++) mbr->partitiontable[i].size = 0;

    //check boot signatur
    if ((src[510] == 0x55) && (src[511] == 0xAA)) {
        //read partition table
        uint16_t baseAddress = 0x01BE;
        for (int i=0; i<4; i++) {
            uint32_t startblock = 0;
            uint32_t size = 0;
            startblock += src[baseAddress + 11] << 24;
            startblock += src[baseAddress + 10] << 16;
            startblock += src[baseAddress +  9] << 8;
            startblock += src[baseAddress +  8];
            size += src[baseAddress + 15] << 24;
            size += src[baseAddress + 14] << 16;
            size += src[baseAddress + 13] << 8;
            size += src[baseAddress + 12];
            mbr->partitiontable[i].block = startblock;
            mbr->partitiontable[i].size = size;
            mbr->partitiontable[i].status = src[baseAddress + 0];
            mbr->partitiontable[i].type = src[baseAddress + 4];
            if (startblock > 0) mbr->partitions++;

            baseAddress += 16;
        }
    }
}

/*--------------------------------------------------------------------------------------------------------
 Accesses the card, reads block zero, and parse the partition information
 ---------------------------------------------------------------------------------------------------------*/
Z80SDCard::mbrResult Z80SDCard::readMBR() {
    mbrResult result;
    //access the card
    result.accessresult = accessCard(true);
    if (result.accessresult == ok) {
        //read block zero
        result.readresult = readBlock(0, sdDataBuffer);
        if (result.readresult == ok) {
            //parse the block
            parseMBR(&result, sdDataBuffer);
        }
    }
    accessCard(false);
    return result;
}

/*--------------------------------------------------------------------------------------------------------
 Initialize and access the card 
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
 Z80 may need to be reset when completed, SPI changes the output buffers uncontrolled. It's up to the context
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
        for (uint16_t i=0; i<1500; i++) {
            sdCommand((uint8_t*)cmd55, 6, 1);
            if (sdCmdRxBuffer[0] == 0x01) {
                sdCommand((uint8_t*)acmd41, 6, 1);
                if (sdCmdRxBuffer[0] == 0x00) {
                    cardReady = true;
                    break;
                }
            }
            if (!cardReady) delayMicroseconds(1000);
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

    //build the read command
    uint8_t sdcmd[SD_COMMAND_BUFFER_LENGTH];
    sdcmd[0] = 17 | 0x40;
    sdcmd[1] = blockNumber >> 24;
    sdcmd[2] = blockNumber >> 16;
    sdcmd[3] = blockNumber >>  8;
    sdcmd[4] = blockNumber;
    sdcmd[5] = 0 | 0x01;

    //control the ssel manually, not via sdCommand
    selectCard(false);
    //send the command to the card
    sdCommand(sdcmd, 6, 1, 15, false);
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
    - wait for complete message from card
---------------------------------------------------------------------------------------------------------*/
Z80SDCard::sdResult Z80SDCard::writeBlock(uint32_t blockNumber, uint8_t* src) {

    //build the write command
    uint8_t sdcmd[SD_COMMAND_BUFFER_LENGTH];
    sdcmd[0] = 24 | 0x40;
    sdcmd[1] = blockNumber >> 24;
    sdcmd[2] = blockNumber >> 16;
    sdcmd[3] = blockNumber >>  8;
    sdcmd[4] = blockNumber;
    sdcmd[5] = 0 | 0x01;

    //control the ssel manually, not via sdCommand
    selectCard(false);
    //send the command to the card
    sdCommand(sdcmd, 6, 1, 15, false);

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
    selectCard(false);
    cardComplete = false;
    for (int i=0; i<10000; i++) {
        lastRx = z80spi.readByte();
        if (lastRx == 0xFF) {
            cardComplete = true;
            break;
        }
    }
    selectCard(true);
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
        if (!(sdCmdRxBuffer[0] & 0x80)) break;
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
