#ifndef Z80_TEST_PROGRAM
#define Z80_TEST_PROGRAM

#include <Arduino.h>

struct z80TestProgram_t {
     const char* name;
     const uint16_t length;
     const uint8_t* data;
};

const uint8_t blinkFlash[] {
     0x21, 0x00, 0x00, 0x3E, 0x00, 0xD3, 0x10, 0x3E, 0x01, 0xD3, 0x20, 0x2B, 0x7C, 0xB5, 0xC2, 0x0B, 
     0x00, 0x3E, 0x04, 0xD3, 0x10, 0x2B, 0x7C, 0xB5, 0xC2, 0x15, 0x00, 0x3E, 0x00, 0xD3, 0x10, 0x3E, 
     0x02, 0xD3, 0x20, 0x2B, 0x7C, 0xB5, 0xC2, 0x23, 0x00, 0x3E, 0x04, 0xD3, 0x10, 0x2B, 0x7C, 0xB5, 
     0xC2, 0x2D, 0x00, 0x3E, 0x00, 0xD3, 0x10, 0x3E, 0x04, 0xD3, 0x20, 0x2B, 0x7C, 0xB5, 0xC2, 0x3B, 
     0x00, 0x3E, 0x04, 0xD3, 0x10, 0x2B, 0x7C, 0xB5, 0xC2, 0x45, 0x00, 0x3E, 0x00, 0xD3, 0x10, 0x3E, 
     0x08, 0xD3, 0x20, 0x2B, 0x7C, 0xB5, 0xC2, 0x53, 0x00, 0x3E, 0x04, 0xD3, 0x10, 0x2B, 0x7C, 0xB5, 
     0xC2, 0x5D, 0x00, 0xC3, 0x03, 0x00, 0x21, 0x00, 0x00, 0x2B, 0x7C, 0xB5, 0xC2, 0x69, 0x00, 0xC9
};

const uint8_t blinkRam[] {
     0x3E, 0x00, 0xD3, 0x10, 0x3E, 0x01, 0xD3, 0x20, 0x21, 0x00, 0x00, 0x11, 0x00, 0x00, 0x01, 0x7F, 
     0x00, 0xED, 0xB0, 0xDB, 0x70, 0x31, 0x00, 0x00, 0xCD, 0x5F, 0x00, 0xCD, 0x69, 0x00, 0xCD, 0x5F, 
     0x00, 0xCD, 0x6E, 0x00, 0x3A, 0x7C, 0x00, 0xE6, 0x01, 0xCA, 0x43, 0x00, 0x3A, 0x7D, 0x00, 0xCB, 
     0x1F, 0xE6, 0x0F, 0xCA, 0x39, 0x00, 0xC3, 0x57, 0x00, 0x3E, 0x00, 0x32, 0x7C, 0x00, 0x3E, 0x02, 
     0xC3, 0x57, 0x00, 0x3A, 0x7D, 0x00, 0xCB, 0x17, 0xE6, 0x0F, 0xCA, 0x50, 0x00, 0xC3, 0x57, 0x00, 
     0x3E, 0x01, 0x32, 0x7C, 0x00, 0x3E, 0x04, 0x32, 0x7D, 0x00, 0xD3, 0x20, 0xC3, 0x18, 0x00, 0x21, 
     0x00, 0x50, 0x2B, 0x7C, 0xB5, 0xC2, 0x62, 0x00, 0xC9, 0x3E, 0x04, 0xD3, 0x10, 0xC9, 0x3E, 0x00, 
     0xD3, 0x10, 0x3A, 0x7E, 0x00, 0x3C, 0x32, 0x7E, 0x00, 0xD3, 0x60, 0xC9, 0x01, 0x01, 0x00     
};

const z80TestProgram_t testPrograms[] = {
     { "BLINK 1 - FLASH - Blink running from Flash", 112, blinkFlash },
     { "BLINK 2 - SRAM  - Blink running from SRAM" , 127, blinkRam }
};

#endif
