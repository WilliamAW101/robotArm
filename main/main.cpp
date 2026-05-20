#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "LED.h"
#include "Camera.h"

static const char *TAG = "RobotArmMain";

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "Program started...");

    LED led(GPIO_NUM_2);
    Camera camera;

    camera.init();
    camera.initSD();

    led.blink(3, 500);

    // int totalFrames = 60;
    
    // // Opening file pointer to record
    // FILE* f = fopen("/sdcard/video2.avi", "ab");
    // if (!f) {
    //     ESP_LOGE(TAG, "fopen failed, errno: %d (%s)", errno, strerror(errno));
    // }
    // else {
    //     for (int i = 0; i < totalFrames; i++) {
    //         camera_fb_t *pic = camera.captureFrame();
    //         if (pic) {
    //             camera.recordVideoToDisk(pic, f);
    //             camera.returnFrame(pic);
    //             if (i % 10 == 0) {
    //                 fflush(f);
    //             }
    //         }
    //         else {
    //             continue;
    //         }
    //         vTaskDelay(pdMS_TO_TICKS(100)); 
    //     }
    // }

    // ESP_LOGI(TAG, "Flushing and closing file...");
    // fflush(f);
    // vTaskDelay(pdMS_TO_TICKS(200)); // Gives FreeRTOS breathing room to prevent watchdog reset
    // fclose(f);
    camera.publishROSTopicCam();
    ESP_LOGI(TAG, "Program Finished.");
}