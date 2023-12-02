#ifndef BUTTON_H
#define BUTTON_H    

    #include <Arduino.h>   

    class Button {

        public:
            Button(uint8_t pin, uint8_t debounceTime_ms = 5);
            void process();
            bool pushTrigger();

        private:
            uint8_t _pin;
            uint8_t _debounceTime;
            uint32_t _lastLevelChange;
            bool _state, _trigger;
            
    };

#endif