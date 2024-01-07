/* -------------------------------------------------------------------------------------------------------
 Magic Sentence

 Compares strings received through serial port with predefined magic sentences, which when reveived
 completely, can start special functions, like entering stm32 DFU mode or starting Z80 flash mode

 Author: Christian Luethi
--------------------------------------------------------------------------------------------------------- */

#ifndef BOOTLOADER_H
#define BOOTLOADER_H

    #include <Arduino.h>

    extern "C" {
    void bootloader_magicSentence(uint8_t c);
    }

#endif