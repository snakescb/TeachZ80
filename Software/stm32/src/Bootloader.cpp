/* -------------------------------------------------------------------------------------------------------
 Bootloader

 When te magic sentence is received through the serial port, the MCU reboots automatically to the
 built-in stm32 bootloader to enable software updates through the stmLoader python script

 Author: Christian Luethi
--------------------------------------------------------------------------------------------------------- */

#include <Bootloader.h>

#define BOOTLOADER_MAGIC_MEMORY_ADDRESS     0x20000200
#define BOOTLOADER_MAGIC_MEMORY_KEY         0xDEADBEEF

const uint8_t magicSentence[] = "heySTM32StartYourDFUMode";
uint16_t magicSentenceCounter = 0;

extern "C" {

/* -------------------------------------------------------------------------------------------------------
 This function receives characters from the serial port. if the magic sentence is received, it updates
 the rtc backup register, and is rebooting the chip
--------------------------------------------------------------------------------------------------------- */
void bootloader_receiver(uint8_t c) {

    if (c == magicSentence[magicSentenceCounter]) {
        magicSentenceCounter++;
        if (magicSentenceCounter == sizeof(magicSentence) - 1) {
            // Magic sentence received completely. 
            // Update the memory location with the bootloader key and reboot
            SCB_DisableDCache();
            *((unsigned long *) BOOTLOADER_MAGIC_MEMORY_ADDRESS) = BOOTLOADER_MAGIC_MEMORY_KEY;
            __DSB();
            NVIC_SystemReset();
        }
    }
    else magicSentenceCounter = 0;

}

/* extern "C" */
}
