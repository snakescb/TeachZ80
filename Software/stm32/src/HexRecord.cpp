#include <HexRecord.h>

/* Types and definitions -------------------------------------------------------------------------------- */  

/*--------------------------------------------------------------------------------------------------------
 Constructor, creates empty instance
---------------------------------------------------------------------------------------------------------*/
HexRecord::HexRecord() {}

/*--------------------------------------------------------------------------------------------------------
 Constructor, sets the data
---------------------------------------------------------------------------------------------------------*/
HexRecord::HexRecord(uint8_t type, uint16_t address, uint8_t* pPayload, uint8_t payloadLength) {
    data(type, address, pPayload, payloadLength);
}

/*--------------------------------------------------------------------------------------------------------
 setData, to update data
---------------------------------------------------------------------------------------------------------*/
void HexRecord::data(uint8_t type, uint16_t address, uint8_t* pPayload, uint8_t payloadLength) {
    this->type = type;
    this->address = address;
    this->payloadLength = payloadLength;
    for (int i=0; i<payloadLength; i++) this->payload[i] = pPayload[i];
}

/*--------------------------------------------------------------------------------------------------------
 byte per byte update
---------------------------------------------------------------------------------------------------------*/
HexRecord::parseResult HexRecord::rxUpdate(uint8_t c) {
    switch (parsestate) {
        case start: {
            if (c == ':') {                
                charCounter = 0;   
                checkSum = 0;
                parsestate = readLengt;
            }
            return incomplete;
        }

        case readLengt: {
            dataBuffer[charCounter++] = c;
            if (charCounter == 2) {
                charCounter = 0;
                payloadLength = charsToNumber(2);   
                checkSum += payloadLength;             
                parsestate = readAddress;
                if (payloadLength > HEX_RECORD_MAX_LENGTH) {                    
                    parsestate = start;       
                    return error;         
                }
            }
            return incomplete;
        }

        case readAddress: {
            dataBuffer[charCounter++] = c;
            if (charCounter == 4) {
                charCounter = 0;
                address = charsToNumber(4);  
                checkSum += address >> 8;
                checkSum += address & 0xFF;                  
                parsestate = readType;                
            }
            return incomplete;
        }

        case readType: {
            dataBuffer[charCounter++] = c;
            if (charCounter == 2) {
                charCounter = 0;
                byteCounter = 0;
                type = charsToNumber(2); 
                checkSum += type;               
                if(payloadLength > 0) parsestate = readData;                
                else parsestate = readChecksum; 
            }
            return incomplete;
        }

        case readData: {
            dataBuffer[charCounter++] = c;
            if (charCounter == 2) {
                charCounter = 0;
                uint8_t value = charsToNumber(2);
                checkSum += value;
                payload[byteCounter++] = value;
                if (byteCounter == payloadLength) {
                    parsestate = readChecksum;
                }                                
            }
            return incomplete;
        }

        case readChecksum: {
            dataBuffer[charCounter++] = c;
            if (charCounter == 2) {
                uint8_t checkSumReceived = charsToNumber(2);
                checkSum = ~checkSum + 1;
                parsestate = start; 
                if (checkSum == checkSumReceived) return valid;
                return error;                                         
            }
            return incomplete;
        }
    }

    return error;
}

/*--------------------------------------------------------------------------------------------------------
 create hex string from stored data
---------------------------------------------------------------------------------------------------------*/
uint8_t HexRecord::getString(uint8_t* pDestBuffer) {
    uint8_t counter = 0;
    checkSum = 0;
    pDestBuffer[counter++] = ':';
    numberToChars(payloadLength, &pDestBuffer[counter], 1);
    counter += 2;
    checkSum += payloadLength;
    numberToChars(address, &pDestBuffer[counter], 2);
    counter += 4;
    checkSum += address >> 8;
    checkSum += address & 0xFF;
    numberToChars(type, &pDestBuffer[counter], 1);
    counter += 2;
    checkSum += type;
    for (int i=0; i<payloadLength; i++) {
        numberToChars(payload[i], &pDestBuffer[counter], 1);
        counter += 2;
        checkSum += payload[i];
    }
    checkSum = ~checkSum + 1;
    numberToChars(checkSum, &pDestBuffer[counter], 1);
    counter += 2;
    return counter;
}

/*--------------------------------------------------------------------------------------------------------
 converts data in buffer to integer, MSB first
---------------------------------------------------------------------------------------------------------*/
unsigned long HexRecord::charsToNumber(uint8_t numBytes) {
    unsigned long result = 0;

    for (int i=0; i<numBytes; i++) {
        uint8_t c = dataBuffer[i];
        if (c >= 'a') c = c - 'a' + 10;
        else if (c >= 'A') c = c - 'A' + 10;
        else if (c >= '0') c = c - '0';

        result  = result*16;
        result += c;
    }

    return result;
}

/*--------------------------------------------------------------------------------------------------------
 converts integer to a string in buffer, MSB first, works for 1 - 4 bytes
---------------------------------------------------------------------------------------------------------*/
void HexRecord::numberToChars(uint16_t number, uint8_t* buffer, uint8_t numBytes) {
    uint32_t numChars = numBytes << 1;
    uint32_t mask = 0xF0 << (numBytes - 1)*8;
    uint32_t shift = (numBytes - 1)*8 + 4;

    for (int i=0; i<numChars; i++) {
        uint32_t nibble = (number & mask) >> shift;
        mask = mask >> 4;
        shift = shift - 4;

        if (nibble >= 10) nibble = nibble - 10 + 'A';
        else nibble = nibble + '0';
        buffer[i] = nibble;
    }
}

/*--------------------------------------------------------------------------------------------------------
 resets the current reception process
---------------------------------------------------------------------------------------------------------*/
void HexRecord::rxReset() {
    parsestate = start;
}