#include "LED.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_mac.h"

LED::LED(gpio_num_t p) : pin(p), state(false) { // after the : is an initializer list
    gpio_reset_pin(pin);
    gpio_set_direction(pin, GPIO_MODE_OUTPUT);
    gpio_set_level(pin, 0);
}

void LED::on() {
    state = true;
    gpio_set_level(pin, 1);
}

void LED::off(){
    state = false;
    gpio_set_level(pin, 0);
}

void LED::toggle() {
    state = !state;
    if (state) { // on
        gpio_set_level(pin, 1);
        return;
    } // off
    gpio_set_level(pin, 0);
}

void LED::blink( int times, int delayInMs) {
    for (int i = 0; i < times; i++) {
        on();
        vTaskDelay(pdMS_TO_TICKS(delayInMs));
        off();
        vTaskDelay(pdMS_TO_TICKS(delayInMs));
    }
}

bool LED::getState() {
    return state;
}