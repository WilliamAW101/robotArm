#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "LED.h"  // Include your header

static const char *TAG = "EXPERIMENT";

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "ESP32 Experiments Started!");
    
    // Create LED object
    LED led(GPIO_NUM_2);
    
    // Main loop
    while(1) {
        led.blink(3, 500);  // Blink 3 times, 500ms each
        vTaskDelay(pdMS_TO_TICKS(2000));  // Wait 2 seconds
    }
}