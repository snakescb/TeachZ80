/* -------------------------------------------------------------------------------------------------------
 TeachZ80 Console
 
 Author: Christian Luethi
--------------------------------------------------------------------------------------------------------- */

#ifndef CONSOLE_H
#define CONSOLE_H

    #include <Arduino.h>      

    class Console {
            
        public:
            Console();    
            void begin();
            void process();
            void serialUpdate(uint8_t c); 

        private:            

            enum menuState: uint8_t { 
                welcome, main, 
                clocks, clockz80, clocksioa, clocksiob, 
                flash, flashdump, flashdumpmin, flashdumpmax, flashdumpresult, flasherase, flasheraseresult, flashselectestprogram, flashtestprogramresult, flashinfo };
            menuState menustate, lastmenustate;

            uint32_t linecount;     
            char textbuffer[64];  
            char inputbuffer[64];
            uint32_t currentInput;
            bool inputModeHex;
            uint32_t debug;
            uint32_t lastFunctionResult;

            void drawMenu();
            void fillScreen();
            void clearScreen();
            void drawLine(const char* text = "");
            void drawLineFormat(const char* text, ...);
            bool addInputChar(uint8_t c, uint8_t maxlen = 8);
            void removeInputChar();

    };

#endif