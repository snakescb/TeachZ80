#include <Z80IO.h>

/* Types and definitions -------------------------------------------------------------------------------- */  

/*--------------------------------------------------------------------------------------------------------
 Constructor
---------------------------------------------------------------------------------------------------------*/
Z80IO::Z80IO(Z80Bus bus) : z80bus(bus) {

}

/*--------------------------------------------------------------------------------------------------------
 IO Write Access to the bus
---------------------------------------------------------------------------------------------------------*/
void Z80IO::write(uint8_t ioport, uint8_t data) {
    z80bus.write_addressBus(ioport);
    z80bus.write_dataBus(data);
    z80bus.write_controlBit(z80bus.ioreq, false);
    z80bus.write_controlBit(z80bus.wr, false);
    z80bus.write_controlBit(z80bus.wr, true);
    z80bus.write_controlBit(z80bus.ioreq, true);
    z80bus.release_dataBus();
}

/*--------------------------------------------------------------------------------------------------------
 IO Read Access to the bus
---------------------------------------------------------------------------------------------------------*/
uint8_t Z80IO::read(uint8_t ioport) {
    z80bus.write_addressBus(ioport);
    z80bus.write_controlBit(z80bus.ioreq, false);
    z80bus.write_controlBit(z80bus.rd, false);
    uint8_t data = z80bus.read_dataBus();
    z80bus.write_controlBit(z80bus.rd, true);
    z80bus.write_controlBit(z80bus.ioreq, true);
    return data;
}

/*--------------------------------------------------------------------------------------------------------
 Request the Z80 bus
---------------------------------------------------------------------------------------------------------*/
void Z80IO::requestBus(bool request) {
    if (request) z80bus.request_bus();
    else z80bus.release_bus();
}
  