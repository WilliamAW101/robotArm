#include "Camera.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "esp_http_server.h"
#include "esp_timer.h"

#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

// AI-Thinker ESP32-CAM PIN Map
#define CAM_PIN_PWDN     32
#define CAM_PIN_RESET    -1  // Software reset
#define CAM_PIN_XCLK      0
#define CAM_PIN_SIOD     26  // SDA
#define CAM_PIN_SIOC     27  // SCL

#define CAM_PIN_D7       35
#define CAM_PIN_D6       34
#define CAM_PIN_D5       39
#define CAM_PIN_D4       36
#define CAM_PIN_D3       21
#define CAM_PIN_D2       19
#define CAM_PIN_D1       18
#define CAM_PIN_D0        5
#define CAM_PIN_VSYNC    25
#define CAM_PIN_HREF     23
#define CAM_PIN_PCLK     22

static const char *TAG = "Camera";

Camera::Camera() : initialized(false), sensor(nullptr) {
    // empty for now maybe
}

camera_config_t Camera::getDefaultConfig() {
    camera_config_t config = {};
    
    config.pin_pwdn = CAM_PIN_PWDN;
    config.pin_reset = CAM_PIN_RESET;
    config.pin_xclk = CAM_PIN_XCLK;
    config.pin_sccb_sda = CAM_PIN_SIOD;
    config.pin_sccb_scl = CAM_PIN_SIOC;
    
    config.pin_d7 = CAM_PIN_D7;
    config.pin_d6 = CAM_PIN_D6;
    config.pin_d5 = CAM_PIN_D5;
    config.pin_d4 = CAM_PIN_D4;
    config.pin_d3 = CAM_PIN_D3;
    config.pin_d2 = CAM_PIN_D2;
    config.pin_d1 = CAM_PIN_D1;
    config.pin_d0 = CAM_PIN_D0;
    config.pin_vsync = CAM_PIN_VSYNC;
    config.pin_href = CAM_PIN_HREF;
    config.pin_pclk = CAM_PIN_PCLK;
    
    config.xclk_freq_hz = 20000000;
    config.ledc_timer = LEDC_TIMER_0;
    config.ledc_channel = LEDC_CHANNEL_0;
    
    config.pixel_format = PIXFORMAT_JPEG;
    
    // CRITICAL: Start with smaller frame size
    config.frame_size = FRAMESIZE_VGA;  // 640x480 instead of SVGA
    
    config.jpeg_quality = 10;  // Lower quality = less memory
    
    // CRITICAL: Use 2 buffers for better performance
    config.fb_count = 2;
    
    // CRITICAL: Explicitly set PSRAM location
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
    
    config.sccb_i2c_port = 1;
    
    return config;
}

esp_err_t Camera::init() {
    camera_config_t config = getDefaultConfig();
    return init(config);
}

esp_err_t Camera::init(const camera_config_t& config) {
    if (initialized) {
        ESP_LOGW(TAG, "Camera already initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    ESP_LOGI(TAG, "Initializing camera...");
    vTaskDelay(pdMS_TO_TICKS(100));
    
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Camera init failed with error 0x%x", err);
        return err;
    }

    ESP_LOGI(TAG, "Initialized camera");
    
    sensor = esp_camera_sensor_get();
    if (!sensor) {
        ESP_LOGE(TAG, "Failed to get camera sensor");
        esp_camera_deinit();
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Received Sensor ID: 0x%02X", sensor->id.PID);
    
    initialized = true;
    ESP_LOGI(TAG, "Camera initialized successfully");
    
    return ESP_OK;
}

camera_fb_t* Camera::captureFrame() {
    if (!initialized) {
        ESP_LOGE(TAG, "Camera not initialized");
        return nullptr;
    }

    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) {
        ESP_LOGE(TAG, "Failed to capture frame");
        return nullptr;
    }

    ESP_LOGI(TAG, "Captured frame: %ux%u, size: %u bytes", fb->width, fb->height, fb->len);

    return fb;
}

esp_err_t jpg_stream_httpd_handler(httpd_req_t *req){
    camera_fb_t * fb = NULL;
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len;
    uint8_t * _jpg_buf;
    char * part_buf[64];
    static int64_t last_frame = 0;
    if(!last_frame) {
        last_frame = esp_timer_get_time();
    }

    res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
    if(res != ESP_OK){
        return res;
    }

    while(true){
        fb = esp_camera_fb_get();
        if (!fb) {
            ESP_LOGE(TAG, "Camera capture failed");
            res = ESP_FAIL;
            break;
        }
        if(fb->format != PIXFORMAT_JPEG){
            bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
            if(!jpeg_converted){
                ESP_LOGE(TAG, "JPEG compression failed");
                esp_camera_fb_return(fb);
                res = ESP_FAIL;
            }
        } else {
            _jpg_buf_len = fb->len;
            _jpg_buf = fb->buf;
        }

        if(res == ESP_OK){
            res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
        }
        if(res == ESP_OK){
            size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);

            res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
        }
        if(res == ESP_OK){
            res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
        }
        if(fb->format != PIXFORMAT_JPEG){
            free(_jpg_buf);
        }
        esp_camera_fb_return(fb);
        if(res != ESP_OK){
            break;
        }
        int64_t fr_end = esp_timer_get_time();
        int64_t frame_time = fr_end - last_frame;
        last_frame = fr_end;
        frame_time /= 1000;
        ESP_LOGI(TAG, "MJPG: %uKB %ums (%.1ffps)",
            (uint32_t)(_jpg_buf_len/1024),
            (uint32_t)frame_time, 1000.0 / (uint32_t)frame_time);
    }

    last_frame = 0;
    return res;
}