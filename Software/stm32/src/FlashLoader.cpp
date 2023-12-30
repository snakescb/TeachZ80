#include <FlashLoader.h>
#include <z80Programs.h>

/* Types and definitions -------------------------------------------------------------------------------- */  
// Timeout for automatic aborting flash mode
#define FLASHLOADER_TIMEOUT_s       10

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
FlashLoader::FlashLoader(Z80Flash flash) : z80flash(flash) {
    loadermode = inactive;
}

/*--------------------------------------------------------------------------------------------------------
 process, run in main loop 
---------------------------------------------------------------------------------------------------------*/
void FlashLoader::process(void) {

    if ((loadermode == active) && (timer != 0)) {
        if (millis() - timer > FLASHLOADER_TIMEOUT_s * 1000) setMode(false);
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
                if (hexCounter == 0) z80flash.eraseFlash();
                hexCounter++;  

                bool writeOK = true;                         
                //write data to flash
                for (int i=0; i<rxHex.payloadLength; i++) z80flash.writeByte(rxHex.address + i, rxHex.payload[i]);                    
                //verify data
                for (int i=0; i<rxHex.payloadLength; i++) if(z80flash.readByte(rxHex.address + i) != rxHex.payload[i]) writeOK = false;

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
                setMode(false);            
            }

            break;
        }

    }
}

/*--------------------------------------------------------------------------------------------------------
 to start and stop flash mode
---------------------------------------------------------------------------------------------------------*/
void FlashLoader::setMode(bool modeactive) {
    if (modeactive) {    
        z80flash.setMode(true);  
        loadermode = active;
        hexCounter = 0;
        rxHex.rxReset();        
        timer = millis();        
    }
    else {
        z80flash.setMode(false); 
        loadermode = inactive;
    }
}