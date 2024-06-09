/* -------------------------------------------------------------------------------------------------------
 Magic Sentence

 Compares strings received through serial port with predefined magic sentences, which when reveived
 completely, can start special functions, like entering stm32 DFU mode or starting Z80 flash mode

 Author: Christian Luethi
--------------------------------------------------------------------------------------------------------- */

#include <Bootloader.h>

#define BOOTLOADER_MAGIC_MEMORY_ADDRESS     0x20000200
#define BOOTLOADER_MAGIC_MEMORY_KEY         0xDEADBEEF

const uint8_t blMagicSentence[]  = "helloStmDFU";
uint16_t blCcounter = 0;

extern "C" {
/* -------------------------------------------------------------------------------------------------------
 This function receives characters from the serial port. if the magic sentence is received, it executes
 the related function
--------------------------------------------------------------------------------------------------------- */
void bootloader_magicSentence(uint8_t c) {

    if (c == blMagicSentence[blCcounter]) {        
        blCcounter++;
        if (blCcounter == sizeof(blMagicSentence) - 1) {
            // Magic sentence received completely. 
            // Update the memory location with the bootloader key and reboot
            SCB_DisableDCache();
            *((unsigned long *) BOOTLOADER_MAGIC_MEMORY_ADDRESS) = BOOTLOADER_MAGIC_MEMORY_KEY;
            __DSB();
            NVIC_SystemReset();
            Serial.println("ok");
        }
    }
    else blCcounter = 0;
}

/* extern "C" */
}
