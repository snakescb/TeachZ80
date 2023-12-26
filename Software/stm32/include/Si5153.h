/* -------------------------------------------------------------------------------------------------------
 Library to control Si5153 frequency generator via soft-I2C

 https://www.skyworksinc.com/-/media/Skyworks/SL/documents/public/application-notes/AN619.pdf
 
 This library is not using the full functional scope of the Si5153 chip, and is using some shortcuts:
    - Uses <SoftWire> software I2C, but library can be easily adjusted to other SW/HW I2C libraries
    - PLLA and PLLB are set to fixed 600MHz (gives best resolution for the low frequencies we use)
    - PLLA and PLLB are driven from the external crystal
    - Assumes a 25MHz crystal and uses the default 10pF load capacitance
    - Spread Spectrum is disabled
    - Clocks drive strenght is set to 8mA
    - Not using integer mode in this implementation (could improve jitter if important)
    - Disabled clock are always low
    - Clocks are not inverted
    - Divide by 4 is not used
 
 Author: Christian Luethi
--------------------------------------------------------------------------------------------------------- */

#ifndef SI5153_H
#define SI5153_H  

    #include <Arduino.h>
    #include <SoftWire.h>

    #define SI5351_I2C_ADDRESS    0x60
    #define SI5351_I2C_BUFSIZE      16

    class Si5153 {
        public:

            enum pll: uint8_t { PLLA = 0, PLLB = 1};
            enum divider: uint8_t { DIV1 = 1, DIV2 = 2, DIV4 = 4, DIV8 = 8, DIV16 = 16, DIV32 = 32, DIV64 = 64, DIV129 = 128 };
            enum enable: uint8_t { ENABLE = 1, DISABLE = 0};

            Si5153(uint8_t sda, uint8_t scl, uint8_t i2cDeviceAddress = SI5351_I2C_ADDRESS);
            bool begin();
            bool configureChannel(uint8_t channel, unsigned long freqOut, uint8_t divider=1, uint8_t pll=0, bool enabled=true);    
            void scanBus(uint8_t firstAddr = 0, uint8_t lastAddr = 0x7F);      
            unsigned long getCurrentClock(uint8_t channel);

        private:
            uint8_t i2caddr, i2cdata[SI5351_I2C_BUFSIZE];
            unsigned long p1, p2, p3, r0;
            unsigned long clocks[8];
            SoftWire i2cbus;
            void i2c_init();
            bool i2c_writeRegister(uint8_t regaddr, uint8_t regbyte);
            bool i2c_writeRegister(uint8_t regaddr, uint8_t* data, uint8_t length);
            bool i2c_readRegister(uint8_t regaddr, uint8_t* data, uint8_t length);   
            void calc_frequencyRegisters(unsigned long clockIn, unsigned long clockOut);         
            bool set_frequencyRegisters(uint8_t baseRegister);         
    };

#endif



