#ifndef LED_H
#define LED_H

#include "driver/gpio.h"

class LED {
    private:
        gpio_num_t pin;
        bool state;

    public:
        LED(gpio_num_t p);
        void on();
        void off();
        void toggle();
        void blink(int times, int delay_ms);
        bool getState();
};

#endif