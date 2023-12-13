#ifndef Z80_TEST_PROGRAM
#define Z80_TEST_PROGRAM

#include <Arduino.h>

#define Z80_TEST_PROGRAM_LENGTH	 26


const uint8_t z80TestProgram[]  {
    0x21, 0x00, 0x00, 0x3E, 0x00, 0xD3, 0x10, 0x2B, 0x7C, 0xB5, 0xC2, 0x07, 0x00, 0x3E, 0x04, 0xD3,
    0x10, 0x2B, 0x7C, 0xB5, 0xC2, 0x11, 0x00, 0xC3, 0x03, 0x00
};

#endif