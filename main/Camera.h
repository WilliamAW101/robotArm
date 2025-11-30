#ifndef CAMERA_H
#define CAMERA_H

#include "esp_err.h" // camera driver
#include "esp_camera.h" // camera error handling
#include "esp_http_server.h"

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

        camera_fb_t* captureFrame();
        esp_err_t jpg_stream_httpd_handler(httpd_req_t *req);
};

#endif