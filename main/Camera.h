#ifndef CAMERA_H
#define CAMERA_H

#include "esp_err.h"
#include "esp_camera.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_common.h"
#include "driver/sdspi_host.h"
#include <errno.h>
#include <string.h>
#include <chrono>
#include <thread>

// Micro-ROS stuff
#include <rmw_microros/rmw_microros.h>
#include <std_msgs/msg/int32.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>

class Camera {
public:
    Camera();
    ~Camera();

    esp_err_t init();
    esp_err_t init(const camera_config_t &config);
    esp_err_t initSD();

    camera_fb_t* captureFrame();
    void returnFrame(camera_fb_t *fb);
    esp_err_t saveFrameToDisk(camera_fb_t *fb);
    esp_err_t recordVideoToDisk(camera_fb_t *fb, FILE *f);
    void publishROSTopicCam();
    static void timer_callback(rcl_timer_t * timer, int64_t last_call_time);

    bool isInitialized() const;
    camera_config_t getDefaultConfig();

    void on();
    void off();

private:
    bool initialized;
    bool sdMounted;
    sensor_t* sensor;
    int frameCount;

    // ROS Stuff
    static rcl_publisher_t publisher;
    static std_msgs__msg__Int32 msg;
};

#endif