/* -------------------------------------------------------------------------------------------------------
 Library for parsing Intel Hex Records
 
 Author: Christian Luethi
--------------------------------------------------------------------------------------------------------- */

#ifndef HEXRECORD_H
#define HEXRECORD_H

    #include <Arduino.h> 

    #define HEX_RECORD_MAX_LENGTH 64
    #define HEX_RECORD_MAX_STRING_LEN  HEX_RECORD_MAX_LENGTH*2 + 11

    class HexRecord {
            
        public:
            enum parseResult: uint8_t { incomplete, valid, error }; 

            uint16_t address;
            uint8_t  type;
            uint8_t  payload[HEX_RECORD_MAX_LENGTH];
            uint8_t  payloadLength;
            uint8_t  checkSum;

            HexRecord();   
            HexRecord(uint8_t type, uint16_t address, uint8_t* pPayload, uint8_t payloadLength); 
            void data(uint8_t type, uint16_t address, uint8_t* pPayload, uint8_t payloadLength);    
            uint8_t getString(uint8_t* pDestBuffer);
            parseResult rxUpdate(uint8_t c);
            void rxReset();

        private:
            enum parseState: uint8_t { start, readLengt, readAddress, readType, readData, readChecksum };
            parseState parsestate;
            uint8_t charCounter;
            uint8_t byteCounter;
            uint8_t dataBuffer[4];
            unsigned long charsToNumber(uint8_t numBytes);
            void numberToChars(uint16_t number, uint8_t* buffer, uint8_t numBytes);
    };

#endif