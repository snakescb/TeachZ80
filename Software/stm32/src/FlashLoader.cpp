#include <FlashLoader.h>

/* Types and definitions -------------------------------------------------------------------------------- */  


/*--------------------------------------------------------------------------------------------------------
 Constructor
---------------------------------------------------------------------------------------------------------*/
FlashLoader::FlashLoader(Z80bus bus) : z80bus(bus) {

}

/*--------------------------------------------------------------------------------------------------------
 process, run in main loop 
---------------------------------------------------------------------------------------------------------*/
void FlashLoader::process(void) {
    

}

/*--------------------------------------------------------------------------------------------------------
 to start and stop flash mode
---------------------------------------------------------------------------------------------------------*/
void FlashLoader::setFlashMode(bool enable_nDisable) {
    if (enable_nDisable) {
        //prepare the bus for use
        flashmode = active;
        z80bus.release_controlBus();
        z80bus.request_bus();
        z80bus.write_controlBit(Z80bus::reset, false);
    }
    else {
        z80bus.release_addressBus();
        z80bus.release_dataBus();
        z80bus.release_controlBus();
        flashmode = inactive;
    }
}

/*--------------------------------------------------------------------------------------------------------
 simple delay method burning some microseconds
---------------------------------------------------------------------------------------------------------*/
void FlashLoader::flashDelay(uint16_t count) {
    //tbd
}