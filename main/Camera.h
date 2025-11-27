#ifndef LED_H
#define LED_H

#include "esp_err.h" // camera driver
#include "esp_camera.h" // camera error handling

class Camera {
    private:
        bool initialized;
        sensor_t* sensor;

    public:
        Camera();
        void on();
        void off();

        esp_err_t init();
        esp_err_t init(const camera_config_t& config); // custom init
        camera_config_t getDefaultConfig();

        void getState();
};

#endif