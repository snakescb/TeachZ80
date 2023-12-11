#ifndef LED_H
#define LED_H    

    #include <Arduino.h>   

    class Led {

        public:
            Led(uint8_t pin);
            void set(uint16_t on, uint16_t off);
            void process();
            void off() { set(0, 1); }
            void on() { set(1, 0); }

        private:
            uint8_t _pin, _state;
            uint16_t _on, _off;
            uint32_t _event;
    };

#endif