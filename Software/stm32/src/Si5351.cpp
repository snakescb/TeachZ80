#include <Si5153.h>

/* Types and definitions -------------------------------------------------------------------------------- */  
#define CHIP_STATUS              0
#define CLK_ENABLE_CONTROL       3
#define PLL_SRC				    15
#define CLK0_CONTROL            16 
#define SYNTH_PLL_A             26
#define SYNTH_PLL_B             34
#define SYNTH_MS_0              42
#define PLL_RESET              177
#define XTAL_LOAD_CAP          183

#define XTAL_FREQ         25000000
#define PLL_FREQ         800000000


/*--------------------------------------------------------------------------------------------------------
 Constructor
---------------------------------------------------------------------------------------------------------*/
Si5153::Si5153(uint8_t sda, uint8_t scl, uint8_t i2cDeviceAddress) : i2cbus(sda, scl) {
    i2caddr = i2cDeviceAddress;
}

/*--------------------------------------------------------------------------------------------------------
 Initalization. Returns true when chip responded as expected
---------------------------------------------------------------------------------------------------------*/
bool Si5153::begin() {

    i2c_init();

    //read the status register. If transfer is successful and bit 7 (SYS_INIT) is 0, the device is ready to be configured
    //if not, delay 10ms and try again, after 5 tries give up
    for (int i=0; i<5; i++) {
        bool i2cOk = i2c_readRegister(CHIP_STATUS, i2cdata, 1);
        if (i2cOk && (i2cdata[0] & 0x80 == 0)) {            
            //device is ready, configure it
            //disable all output clocks
            i2c_writeRegister(CLK_ENABLE_CONTROL, 0xFF);
            //use crystal for both PLL's, divider 1 -> PLL input = 25MHz
            i2c_writeRegister(PLL_SRC, 0);
            //set both PLL's to 800Mhz. Calculate register contents
            calc_frequencyRegisters(XTAL_FREQ, PLL_FREQ);
            //divider is 0 for PLL's
            r0 = 0;
            //setup both PLL's
            set_frequencyRegisters(SYNTH_PLL_A);
            set_frequencyRegisters(SYNTH_PLL_B);
            //configure all 8 clocks to PLLA, multisynth, power down, 8ma drive, non inverted
            for (int i=0; i<8; i++) i2cdata[i] = 0b10001111;
            i2c_writeRegister(CLK0_CONTROL, i2cdata, 8);

            Serial.println("Si5351: Initialization complete");
            return true;
        }
        else delay(10);
    }
    //device not fount
    Serial.println("Si5351: Initialization failed");
    return false;
}

/*--------------------------------------------------------------------------------------------------------
 Setup output channel
 PLL: 0 = PLLA, 1 = PLLB
 Divider: 1-128
 Channel: 0-7
---------------------------------------------------------------------------------------------------------*/
bool Si5153::configureChannel(uint8_t channel, float freqOut, uint8_t divider, uint8_t pll, bool enabled) {
    //check channel
    if (channel > 7) { Serial.println("Si5351: Invalid channel received in configuration"); return false; }
    if (pll > 1) { Serial.println("Si5351: Invalid pll received in configuration"); return false; }
    if ((divider == 0) || (divider > 128)) { Serial.println("Si5351: Invalid divider received in configuration"); return false; }

    //check if output frequency can be achieved by multisynth alone (without divider)
    if (freqOut > PLL_FREQ/15) { Serial.printf("Si5351: Requested frequency is too high. Maximum possible is %d\r\n", PLL_FREQ/15); return false; }
    if (freqOut < PLL_FREQ/90) { Serial.printf("Si5351: Requested frequency is too low. Minimum possible is %d\r\n", PLL_FREQ/90); return false; }

    //disable channel
    //set OEB bit to disable channel
    uint8_t OEB_register;    
    i2c_readRegister(CLK_ENABLE_CONTROL, &OEB_register, 1);
    OEB_register = OEB_register | (0x01 << channel);
    i2c_writeRegister(CLK_ENABLE_CONTROL, OEB_register);
    //reinitialize clock control register, will set power down bit to disable clock
    uint8_t CTRL_register =  0b10001111;
    i2c_writeRegister(CLK0_CONTROL+channel, CTRL_register);

    
    if (!enabled) {
        Serial.printf("Si5351: Channel %d disabled\r\n", channel);
        return true;
    }

    //setup multisynth registers
    calc_frequencyRegisters(PLL_FREQ, freqOut);
    r0 = divider - 1;
    set_frequencyRegisters(SYNTH_MS_0 + channel*8);

    //enable clock
    OEB_register = OEB_register & ~(0x01 << channel);
    i2c_writeRegister(CLK_ENABLE_CONTROL, OEB_register);
    CTRL_register = (CTRL_register & 0x7F) | (pll < 5);
    i2c_writeRegister(CLK0_CONTROL+channel, CTRL_register);

    Serial.printf("Si5351: Channel %d configured and enabled\r\n", channel);
    return true;
}

/*--------------------------------------------------------------------------------------------------------
 Calculates the required p1, p2, p3 values for the frequency divider registers
 R divider IS not considered. The output frequency can be further divides by 1..128 with the r registers.
 user can set R through configureChannel function if further division is required
---------------------------------------------------------------------------------------------------------*/
void Si5153::calc_frequencyRegisters(float clockIn, float clockOut) {
    unsigned long a, b;
    unsigned long c = 0x0FFFFF; // 1048575, full resolution for c (c is a 20 bit number)
    double divider;

    //calulcate a, b and c
    //f_out = f_in*(a + (b/c))
    divider = clockOut / clockIn;
    a = (unsigned long) divider;
    b = (unsigned long) (divider - a)*c;

    //calulate p1, p2 and p3 according datasheet
    p1 = 128*a + ((unsigned int) 128*b / c) - 512;
    p2 = 128*b - ((unsigned int) 128*b / c);
    p3 = c;

    Serial.printf("Si5351 debug: Frequency registers calculated: f_in: %d, f_out: %d - abc: a: %d, b: %d, c: %d - p-values: p1: %d, p2: %d, p3: %d\r\n", clockIn, clockOut, a, b, c, p1, p2, p3);
}  

/*--------------------------------------------------------------------------------------------------------
 puts the previously calulated p1, p2 and p3 in the respective registers (including r0)
---------------------------------------------------------------------------------------------------------*/
bool Si5153::set_frequencyRegisters(uint8_t baseRegister) {
    i2cdata[0] = (p3 & 0x0000FF00) >> 8;
    i2cdata[1] = (p3 & 0x000000FF);
    i2cdata[2] = ((p1 & 0x00030000) >> 16) | (r0 << 4);
    i2cdata[3] = (p1 & 0x0000FF00) >> 8;
    i2cdata[4] = (p1 & 0x000000FF);
    i2cdata[5] = ((p3 & 0x000F0000) >> 12) | ((p2 & 0x000F000) >> 16);
    i2cdata[6] = (p2 & 0x0000FF00) >> 8;
    i2cdata[7] = (p2 & 0x000000FF);

    return i2c_writeRegister(baseRegister, i2cdata, 8);
}

/*--------------------------------------------------------------------------------------------------------
 I2C functions
---------------------------------------------------------------------------------------------------------*/
void Si5153::i2c_init() {
    i2cbus.setTimeout_ms(50);
    i2cbus.begin();
}

bool Si5153::i2c_writeRegister(uint8_t regaddr, uint8_t* data, uint8_t length) {
    if (length > SI5351_I2C_BUFSIZE) return false;

    uint8_t errors = 0;
    //write register address to chip
    errors += i2cbus.startWrite(i2caddr);
    errors += i2cbus.write(regaddr);
    //write data to chip
    for (int i=0; i<length; i++) errors += i2cbus.readThenAck(data[i]);
    i2cbus.stop();

    if (errors) return false;
    return true;
}

bool Si5153::i2c_readRegister(uint8_t regaddr, uint8_t* data, uint8_t length) {
    if (length > SI5351_I2C_BUFSIZE) return false;

    uint8_t errors = 0;
    //write register address to chip, follows stop by datasheet
    errors += i2cbus.startWrite(i2caddr);
    errors += i2cbus.write(regaddr);
    i2cbus.stop();
    //read data from chip
    errors += i2cbus.startRead(i2caddr);
    for (int i=0; i<length-1; i++) errors += i2cbus.readThenAck(data[i]);
    errors += i2cbus.readThenNack(data[length-1]);
    i2cbus.stop();

    if (errors) return false;
    return true;
}

bool Si5153::i2c_writeRegister(uint8_t regaddr, uint8_t regbyte) {
    return i2c_writeRegister(regaddr, &regbyte, 1);
}
