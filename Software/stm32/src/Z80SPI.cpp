#include <Z80SPI.h>

/* Types and definitions -------------------------------------------------------------------------------- */  
#define SPI_OUT_IOPORT      0x10
#define SPI_IN_IOPORT       0x00
#define SPI_OUT_MOSI        0x01
#define SPI_OUT_CLK         0x02
#define SPI_OUT_SSEL        0x04
#define SPI_IN_MISO         0x80
#define SPI_IN_SDDET        0x40

/*--------------------------------------------------------------------------------------------------------
 Constructor
---------------------------------------------------------------------------------------------------------*/
Z80SPI::Z80SPI(Z80IO io) : z80io(io) {
    outputBuffer = SPI_OUT_MOSI | SPI_OUT_SSEL;
}

/*--------------------------------------------------------------------------------------------------------
 Write a single byte
---------------------------------------------------------------------------------------------------------*/
void Z80SPI::writeByte(uint8_t data) {

    uint8_t mask = 0x80;
    for (int i=0; i<8; i++) {
        outputBuffer = outputBuffer & ~SPI_OUT_CLK;
        if (data & mask) outputBuffer = outputBuffer | SPI_OUT_MOSI;
        else outputBuffer = outputBuffer & ~SPI_OUT_MOSI;
        mask = mask >> 1;
        z80io.write(SPI_OUT_IOPORT, outputBuffer);
        outputBuffer = outputBuffer | SPI_OUT_CLK;
        z80io.write(SPI_OUT_IOPORT, outputBuffer);
    }
    outputBuffer = outputBuffer & ~SPI_OUT_CLK;
    outputBuffer = outputBuffer | SPI_OUT_MOSI;
    z80io.write(SPI_OUT_IOPORT, outputBuffer);
}

/*--------------------------------------------------------------------------------------------------------
 Read a single byte
---------------------------------------------------------------------------------------------------------*/
uint8_t Z80SPI::readByte(void) {
    uint8_t result = 0;
    /*
    for (int i=0; i<8; i++) {
        result = result << 1;
        outputBuffer = outputBuffer & ~SPI_OUT_CLK;
        z80io.write(SPI_OUT_IOPORT, outputBuffer);
        outputBuffer = outputBuffer | SPI_OUT_CLK;
        z80io.write(SPI_OUT_IOPORT, outputBuffer);
        uint8_t data = z80io.read(SPI_IN_IOPORT);
        if (data & SPI_IN_MISO) result = result | 0x01;
    }
    outputBuffer = outputBuffer & ~SPI_OUT_CLK;
    z80io.write(SPI_OUT_IOPORT, outputBuffer);
    */
    for (int i=0; i<8; i++) {
        result = result << 1;        
        outputBuffer = outputBuffer | SPI_OUT_CLK;
        z80io.write(SPI_OUT_IOPORT, outputBuffer);
        uint8_t data = z80io.read(SPI_IN_IOPORT);
        if (data & SPI_IN_MISO) result = result | 0x01;
        outputBuffer = outputBuffer & ~SPI_OUT_CLK;
        z80io.write(SPI_OUT_IOPORT, outputBuffer);
    }
    return result;
}

/*--------------------------------------------------------------------------------------------------------
 clears or sets the slave select line
---------------------------------------------------------------------------------------------------------*/
void Z80SPI::slaveSelect(bool state) {
    if (state) outputBuffer = outputBuffer | SPI_OUT_SSEL;
    else outputBuffer = outputBuffer & ~SPI_OUT_SSEL;
    z80io.write(SPI_OUT_IOPORT, outputBuffer);
}

/*--------------------------------------------------------------------------------------------------------
 for simplicity, as this driver has access to the io bus already, it can also pass the sddetect line
---------------------------------------------------------------------------------------------------------*/
bool Z80SPI::checkSDDetect(void) {
    uint8_t inport = z80io.read(SPI_IN_IOPORT);
    return inport & SPI_IN_SDDET;
}

/*--------------------------------------------------------------------------------------------------------
 requestBus
---------------------------------------------------------------------------------------------------------*/
void Z80SPI::requestBus(bool request) {
    z80io.requestBus(request);
    if (request) {
        outputBuffer = SPI_OUT_MOSI | SPI_OUT_SSEL;
        z80io.write(SPI_OUT_IOPORT, outputBuffer);
    }
}