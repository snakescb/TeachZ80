/* -------------------------------------------------------------------------------------------------------
 Bootloader

 When te magic sentence is received through the serial port, the MCU reboots automatically to the
 built-in stm32 bootloader to enable software updates through the stmLoader python script

 Author: Christian Luethi
--------------------------------------------------------------------------------------------------------- */

#ifndef BOOTLOADER_H
#define BOOTLOADER_H

    #include <Arduino.h>

    extern "C" {
    void bootloader_receiver(uint8_t c);
    }

#endif