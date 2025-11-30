#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "LED.h" 
#include "Camera.h"
#include "esp_http_server.h"
#include <sys/param.h>
#include "esp_http_client.h"

static const char *TAG = "RobotArmMain";

esp_err_t send_image_to_computer(camera_fb_t *fb)
{
    esp_http_client_config_t config = {
        .url = "http://192.168.27.128:8000/upload",  // Your computer's IP
        .method = HTTP_METHOD_POST,
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    
    esp_http_client_set_header(client, "Content-Type", "image/jpeg");
    esp_http_client_set_post_field(client, (const char *)fb->buf, fb->len);
    
    esp_err_t err = esp_http_client_perform(client);
    
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Image sent, status=%d", esp_http_client_get_status_code(client));
    }
    
    esp_http_client_cleanup(client);
    return err;
}

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Program started...");
    
    // Create LED object
    LED led(GPIO_NUM_2);

    // Create Camera object
    Camera camera;
    camera.init();

    // Main loop
    while(1) {
        led.blink(3, 500); 
        camera_fb_t *pic = camera.captureFrame();  

        if (pic) {
            esp_err_t err = send_image_to_computer(pic);
            if (err != ESP_OK)
                ESP_LOGI(TAG, "Failed to send image");
        }

        esp_camera_fb_return(pic); 
        vTaskDelay(pdMS_TO_TICKS(2000));  
        exit(0);  // Exit after one iteration for testing
    }
}