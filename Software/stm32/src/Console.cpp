#include <Console.h>
#include <Version.h>
#include <FlashLoader.h>
#include <Si5153.h>
#include <Config.h>
#include <Z80bus.h>
#include <Z80TestPrograms.h>

/* Types and definitions ------------------------------------------------------------------------------- */  
#define SCREEN_HEIGHT      22
#define KEY_ESC            27
#define KEY_LINE_FEED      10
#define KEY_CARRIAGE_FEED  13
#define KEY_BACKSPACE       8
#define KEY_DEL           127

/* String Constants ------------------------------------------------------------------------------------ */  
const char headerDivider[]    =   "**************************************************************";
const char menuDivider[]      =   " --------------------------------------------------";
const char* menuTitles[]    = { " Welcome ", 
                                " TeachZ80 - Main Menu", 
                                " TeachZ80 - Main Menu - Clocks", 
                                " TeachZ80 - Main Menu - Clocks - Z80 Clock", 
                                " TeachZ80 - Main Menu - Clocks - SIO-A Clock", 
                                " TeachZ80 - Main Menu - Clocks - SIO-B Clock",
                                " TeachZ80 - Main Menu - Flash", 
                                " TeachZ80 - Main Menu - Flash - Memory Dump", 
                                " TeachZ80 - Main Menu - Flash - Memory Dump - Start Address",
                                " TeachZ80 - Main Menu - Flash - Memory Dump - End Address",
                                " TeachZ80 - Main Menu - Flash - Memory Dump - Result",
                                " TeachZ80 - Main Menu - Flash - Erase",
                                " TeachZ80 - Main Menu - Flash - Erase",
                                " TeachZ80 - Main Menu - Flash - Testprogram",
                                " TeachZ80 - Main Menu - Flash - Testprogram",
                                " TeachZ80 - Main Menu - Flash - Information"
                              };

/* extern references ----------------------------------------------------------------------------------- */  
extern Si5153 clock;
extern FlashLoader flashloader;   
extern Config config;         
extern Z80bus z80bus;                    

/*--------------------------------------------------------------------------------------------------------
 Constructor
---------------------------------------------------------------------------------------------------------*/
Console::Console() {
    menustate = welcome;
}

/*--------------------------------------------------------------------------------------------------------
 process, run in main loop 
---------------------------------------------------------------------------------------------------------*/
void Console::begin(void) {
    clearScreen();

    drawLine(headerDivider); 
    drawLine(" WELCOME to TeachZ80!");
    drawLine(" Your Z80 Learning Platform");
    drawLine();
    drawLineFormat(" Version: %u.%u", VERSION_MAJOR, VERSION_MINOR);
    drawLine(headerDivider); 
    drawLine();
    drawLine(" Push any button to start console menu");

    fillScreen();
}



/*--------------------------------------------------------------------------------------------------------
 drawMenu
---------------------------------------------------------------------------------------------------------*/
void Console::drawMenu() {
    linecount = 0;

    //draw header
    drawLine(headerDivider);
    drawLine(menuTitles[menustate]);
    drawLine(headerDivider);

    switch (menustate) {
        case main: {
            drawLine("");
            drawLine(" Commands");
            drawLine(menuDivider);
            drawLine(" 1: Clock Menu");
            drawLine(" 2: Flash Menu");
            break;
        }
        case clocks: {
            drawLine("");
            drawLine(" Current Clocks");
            drawLine(menuDivider);
            unsigned long outfreqfull = clock.getCurrentClock(0) / 1000000;
            unsigned long outfreqrem  = (clock.getCurrentClock(0) -  outfreqfull*1000000) / 100;
            drawLineFormat(" Z80 Clock   :  %2d.%04d MHz", outfreqfull, outfreqrem);
            outfreqfull = clock.getCurrentClock(1) / 1000000;
            outfreqrem  = (clock.getCurrentClock(1) -  outfreqfull*1000000) / 100;
            drawLineFormat(" SIO-A Clock :  %2d.%04d MHz", outfreqfull, outfreqrem);
            outfreqfull = clock.getCurrentClock(2) / 1000000;
            outfreqrem  = (clock.getCurrentClock(2) -  outfreqfull*1000000) / 100;
            drawLineFormat(" SIO-B Clock :  %2d.%04d MHz", outfreqfull, outfreqrem);
            drawLine("");
            drawLine(" Commands");
            drawLine(menuDivider);
            drawLine(" 1: Change Z80 Clock");
            drawLine(" 2: Change SIO-A Clock");
            drawLine(" 3: Change SIO-B Clock");
            drawLine(menuDivider);
            drawLine(" 9: Main Menu");
            break;
        }
        case clockz80:
        case clocksioa:
        case clocksiob: {
            uint8_t clockChannel = 0;
            char* description = (char*)" Enter Z80 Clock Value";
            if (menustate == clocksioa) { clockChannel = 1; description = (char*)" Enter SIO-A Clock Value "; }
            if (menustate == clocksiob) { clockChannel = 2; description = (char*)" Enter SIO-B Clock Value "; }
            drawLine("");
            drawLine(description);
            drawLine(menuDivider);
            unsigned long outfreqfull = clock.getCurrentClock(clockChannel) / 1000000;
            unsigned long outfreqrem  = (clock.getCurrentClock(clockChannel) -  outfreqfull*1000000) / 100;
            drawLineFormat(" Current :  %2d.%04d MHz", outfreqfull, outfreqrem);
            outfreqfull = currentInput*100 / 1000000;
            outfreqrem  = (currentInput*100 -  outfreqfull*1000000) / 100;
            drawLineFormat("     New :  %2d.%04d MHz", outfreqfull, outfreqrem);
            drawLine("");
            drawLine(" Commands");
            drawLine(menuDivider);
            drawLine(" Enter: Activate (Temporary)");
            drawLine(" +    : Activate + Save");
            drawLine(" ESC  : Cancel");
            break;
        }
        case flash: {
            drawLine("");
            drawLine(" Commands");
            drawLine(menuDivider);
            drawLine(" 1: Memory Dump");
            drawLine(" 2: Flash Information");
            drawLine(" 3: Erase Flash");
            drawLine(" 4: Write Testprogram");
            drawLine(menuDivider);
            drawLine(" 9: Main Menu");
            break;
        }

        case flashdump: {
            drawLine("");
            drawLine("Memory Dump Range");
            drawLine(menuDivider);
            drawLineFormat(" Start Address :  0x%04X", config.configdata.flash.dumpStart);
            drawLineFormat("   End Address :  0x%04X", config.configdata.flash.dumpEnd);
            drawLine("");
            drawLine(" Commands");
            drawLine(menuDivider);
            drawLine(" 1: Start Dump");
            drawLine(" 2: Change Start Address");
            drawLine(" 3: Change End Address");
            drawLine(menuDivider);
            drawLine(" 9: Main Menu");
            break;
        }

        case flashdumpmin:
        case flashdumpmax: {
            uint16_t addr = config.configdata.flash.dumpStart;
            char* description = (char*)" Enter Start Address";
            if (menustate == flashdumpmax)  { addr = config.configdata.flash.dumpEnd; description = (char*)" Enter End Address"; }
            drawLine("");
            drawLine(description);
            drawLine(menuDivider);
            drawLineFormat(" Current :  0x%04X", addr);
            drawLineFormat("     New :  0x%04X", currentInput);
            drawLine("");
            drawLine(" Commands");
            drawLine(menuDivider);
            drawLine(" Enter: Save");
            drawLine(" ESC  : Cancel");
            break;
            break;
        }

        case flashdumpresult: {
            drawLine("");
            drawLineFormat(" Flash Dump 0x%04X - 0x%04X", config.configdata.flash.dumpStart, config.configdata.flash.dumpEnd);
            drawLine(" -------------------------------------------------------------------------");

            flashloader.setFlashMode(true);
            uint16_t numlines = (config.configdata.flash.dumpEnd + 1 - config.configdata.flash.dumpStart) >> 4;
            uint16_t address = config.configdata.flash.dumpStart;
            uint8_t databuffer[16];
            for (unsigned int i=0; i<numlines; i++) {
                for (int j=0; j<16; j++) databuffer[j] = flashloader.readByte(address + j);
                Serial.printf(" %04X: ", address);
                for (int j=0; j<8; j++) Serial.printf(" %02X", databuffer[j]);
                Serial.print(" ");
                for (int j=8; j<16; j++) Serial.printf(" %02X", databuffer[j]);
                Serial.print("  ");

                for (int j=0; j<16; j++) {
                    if ((databuffer[j] >= 21) && (databuffer[j] < 127)) Serial.write(databuffer[j]);
                    else Serial.write('.');
                }
                drawLine("");
                address += 0x10;
            }
            flashloader.setFlashMode(false);
     
            drawLine(" -------------------------------------------------------------------------");
            drawLine(" 9: Back");
            break;
            break;
        }

        case flasherase: {
            drawLine("");
            drawLine(" PLEASE CONFIRM FLASH ERASE");
            drawLine("");
            drawLine(" Commands");
            drawLine(menuDivider);
            drawLine(" Enter: Confirm");
            drawLine(" ESC  : Cancel");
            break;
        }

        case flasheraseresult: {
            drawLine("");
            if (lastFunctionResult == 0) drawLine(" Flash Erase SUCCESSFUL");
            else drawLineFormat(" Flash Erase ERROR: %u Bytes still programmed", lastFunctionResult);
            drawLine("");
            drawLine(" Commands");
            drawLine(menuDivider);
            drawLine(" 9: Back");
            break;
        }

        case flashselectestprogram: {
            drawLine("");
            drawLine(" Select Test Program to write to FLASH");
            drawLine(" Warning: Flash will be erased and rewritten!");
            drawLine("");            
            drawLine(" Available Programs");
            drawLine(" -------------------------------------------------------------------------");
            for (int i=0; i<sizeof(testPrograms) / sizeof(z80TestProgram_t); i++) {
                Serial.printf(" %u: ", i+1);
                Serial.print(testPrograms[i].name);
                Serial.printf(" (%u Bytes)", testPrograms[i].length);
                drawLine("");
            }
            drawLine(" -------------------------------------------------------------------------");
            drawLine(" 9: Back");
            break;
        }

        case flashtestprogramresult: {
            drawLine("");
            if (lastFunctionResult) drawLine(" Testprogramm written and verified SUCCESSFUL");
            else drawLine(" Testprogram write ERROR: Verification failed");
            drawLine("");
            drawLine(" Commands");
            drawLine(menuDivider);
            drawLine(" 9: Back");
            break;
        }

        case flashinfo: {
            drawLine("");
            drawLine(" Flash Information");
            drawLine(menuDivider);
            if (flashloader.chipVendorId == 0xBF) {
                drawLine(" Vendor     : Microchip");
                if (flashloader.chipDeviceId == 0xB5) {
                    drawLine(" Device     : SST39SF010A");
                    drawLine(" Capacity   : 128kB (128k x 8)");
                }
                else if (flashloader.chipDeviceId == 0xB6) {
                    drawLine(" Device     : SST39SF020A");
                    drawLine(" Capacity   : 256kB (256k x 8)");
                }
                else if (flashloader.chipDeviceId == 0xB7) {
                    drawLine(" Device     : SST39SF040");
                    drawLine(" Capacity   : 512kB (512k x 8)");
                }
                else {
                    drawLine(" Device     : Unknown");
                    drawLine(" Capacity   : Unknown");
                }
            }
            else {
                drawLine(" Vendor     : Unknown");
                drawLine(" Device     : Unknown");
                drawLine(" Capacity   : Unknown");
            }
            drawLineFormat(" Programmed : %u Bytes", lastFunctionResult);
            drawLine("");
            drawLine(" Commands");
            drawLine(menuDivider);
            drawLine(" 9: Back");
            break;
        }

        default: {
            break;
        }
    }
}


/*--------------------------------------------------------------------------------------------------------
 reception of new serial characters
---------------------------------------------------------------------------------------------------------*/
void Console::serialUpdate(uint8_t c) {
    
    bool refreshScreen = true;
    
    switch (menustate) {
        case welcome: {
            menustate = main;
            break;
        }

        case main: {
            if (c == '1') menustate = clocks;
            else if (c == '2') menustate = flash;
            else {
                //Serial.println(c);
                refreshScreen = false;
            }
            break;
        }
        case clocks: {
            inputbuffer[0] = 0;
            inputModeHex = false;
            currentInput = 0;
            if (c == '1') menustate = clockz80;
            else if (c == '2') menustate = clocksioa;
            else if (c == '3') menustate = clocksiob;
            else if (c == '9') menustate = main;
            else refreshScreen = false;
            break;
        }
        case clockz80:
        case clocksioa:
        case clocksiob: {            
            if (c == KEY_ESC) menustate = clocks;
            else if ((c == KEY_BACKSPACE) || (c == KEY_DEL)) removeInputChar();
            else if ((c == KEY_LINE_FEED) || (c == KEY_CARRIAGE_FEED) || (c == '+')) {

                uint8_t clockChannel = 0;
                if (menustate == clocksioa) clockChannel = 1;
                if (menustate == clocksiob) clockChannel = 2;

                if (clockChannel == 0) {                    
                    clock.configureChannel(0, currentInput*100, clock.DIV1, clock.PLLA, clock.ENABLE);
                    if (c == '+') {
                        config.configdata.clock.z80Clock = currentInput*100;
                        config.write();
                    }
                }
                else if (clockChannel == 1) {
                    clock.configureChannel(1, currentInput*100, clock.DIV1, clock.PLLB, clock.ENABLE);
                    if (c == '+') {
                        config.configdata.clock.sioaClock = currentInput*100;
                        config.write();
                    }
                }
                else {
                    clock.configureChannel(2, currentInput*100, clock.DIV1, clock.PLLB, clock.ENABLE);
                    if (c == '+') {
                        config.configdata.clock.siobClock = currentInput*100;
                        config.write();
                    }
                }
                menustate = clocks;
            }
            else refreshScreen = addInputChar(c, 6);
            break;
        }
        case flash: {             
            if (c == '1') menustate = flashdump;
            else if (c == '2') {
                flashloader.setFlashMode(true);
                flashloader.readChipIndentification();
                lastFunctionResult = flashloader.bytesProgrammed();
                flashloader.setFlashMode(false);
                menustate = flashinfo;
            }
            else if (c == '3') menustate = flasherase;
            else if (c == '4') menustate = flashselectestprogram;
            else if (c == '9') menustate = main;
            else refreshScreen = false;
            break;
        }

        case flashdump: {
            inputModeHex = true;
            inputbuffer[0] = 0;
            if (c == '1') menustate = flashdumpresult;
            else if (c == '2') menustate = flashdumpmin;
            else if (c == '3') menustate = flashdumpmax;
            else if (c == '9') menustate = flash;
            else refreshScreen = false;
            break;
        }

        case flashdumpmin:
        case flashdumpmax: {
            if (c == KEY_ESC) menustate = flashdump;
            else if ((c == KEY_BACKSPACE) || (c == KEY_DEL)) removeInputChar();
            else if ((c == KEY_LINE_FEED) || (c == KEY_CARRIAGE_FEED)) {
                if (menustate == flashdumpmin) config.configdata.flash.dumpStart = currentInput & 0xFFF0;
                if (menustate == flashdumpmax) config.configdata.flash.dumpEnd   = ( currentInput & 0xFFFF ) | 0x00F;
                config.write();
                menustate = flashdump;
            }
            else refreshScreen = addInputChar(c, 4);
            break;
        }

        case flashdumpresult: {
            if (c == '9') menustate = flashdump;
            else refreshScreen = false;
            break;
        }

        case flasherase: {
            if (c == KEY_ESC) menustate = flash;
            else if ((c == KEY_LINE_FEED) || (c == KEY_CARRIAGE_FEED)) {
                flashloader.setFlashMode(true);
                flashloader.eraseFlash();
                lastFunctionResult = flashloader.bytesProgrammed();
                flashloader.setFlashMode(false);
                menustate = flasheraseresult;
            }
            else refreshScreen = false;
            break;
        }

        case flasheraseresult: {
            if (c == '9') menustate = flash;
            else refreshScreen = false;
            break;
        }

        case flashselectestprogram: {
            if (c == '9') menustate = flash;
            else {
                if ((c >= '1') && (c <= '8')) {
                    uint8_t programNumber = c - '1';
                    if (programNumber <= (sizeof(testPrograms) / sizeof(z80TestProgram_t)) - 1) { 
                        lastFunctionResult = flashloader.writeTestProgram(programNumber); 
                        menustate = flashtestprogramresult;
                    }
                    else refreshScreen = false;
                }
                else refreshScreen = false;
            }
            break;
        }

        case flashtestprogramresult: {
            if (c == '9') menustate = flash;
            else refreshScreen = false;
            break;
        }

        case flashinfo: {
            if (c == '9') menustate = flash;
            else refreshScreen = false;
            break; 
        }

    }

    if (refreshScreen) {
        clearScreen();        
        drawMenu();
        fillScreen();
    }
}

/*--------------------------------------------------------------------------------------------------------
 string helper functions
---------------------------------------------------------------------------------------------------------*/
bool Console::addInputChar(uint8_t c, uint8_t maxlen) {
    if (!inputModeHex && ((c < '0') || (c > '9'))) return false;
    if ( inputModeHex && ((c < '0') || ((c > '9') && (c < 'A')) || ((c > 'F') && (c < 'a')) || (c > 'f'))) return false;
    int strlen ;
    for (strlen=0; strlen<63; strlen++) if (inputbuffer[strlen] == 0) break;
    if (strlen >= maxlen) return false;
    inputbuffer[strlen] = c;
    inputbuffer[strlen+1] = 0;
    if (!inputModeHex) currentInput = strtol(inputbuffer, NULL, 10);
    if ( inputModeHex) currentInput = strtol(inputbuffer, NULL, 16);
    return true;
}

void Console::removeInputChar() {
    int strlen ;
    for (strlen=0; strlen<63; strlen++) if (inputbuffer[strlen] == 0) break;
    if (strlen > 0) inputbuffer[strlen-1] = 0;
    if (!inputModeHex) currentInput = strtol(inputbuffer, NULL, 10);
    if ( inputModeHex) currentInput = strtol(inputbuffer, NULL, 16);
}

/*--------------------------------------------------------------------------------------------------------
 screen helper functions
---------------------------------------------------------------------------------------------------------*/
void Console::drawLine(const char* text) {
    Serial.println(text);
    linecount++;
}

void Console::drawLineFormat(const char* text, ...) {
    sprintf(textbuffer, text);
    drawLine(textbuffer);
}
 
void Console::fillScreen() {
    for (int i=linecount; i<SCREEN_HEIGHT; i++) Serial.println();
}

/*--------------------------------------------------------------------------------------------------------
 clearScreen
---------------------------------------------------------------------------------------------------------*/
void Console::clearScreen(void) {
    for (int i=0; i<64; i++) Serial.println();
    linecount = 0;
}


/*--------------------------------------------------------------------------------------------------------
 process, run in main loop 
---------------------------------------------------------------------------------------------------------*/
void Console::process(void) {
        
}