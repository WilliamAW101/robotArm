#include "Camera.h"

// AI-Thinker ESP32-CAM PIN Map
#define CAM_PIN_PWDN     32
#define CAM_PIN_RESET    -1
#define CAM_PIN_XCLK      0
#define CAM_PIN_SIOD     26
#define CAM_PIN_SIOC     27
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

#define RCCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){printf("Failed status on line %d: %d. Aborting.\n",__LINE__,(int)temp_rc);vTaskDelete(NULL);}}
#define RCSOFTCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){printf("Failed status on line %d: %d. Continuing.\n",__LINE__,(int)temp_rc);}}

rcl_publisher_t Camera::publisher;
std_msgs__msg__Int32 Camera::msg;

static const char *TAG = "Camera";

Camera::Camera() : initialized(false), sdMounted(false), sensor(nullptr), frameCount(0) {
}

Camera::~Camera() {
    if (initialized) {
        esp_camera_deinit();
    }
}

camera_config_t Camera::getDefaultConfig() {
    camera_config_t config = {};

    config.pin_pwdn     = CAM_PIN_PWDN;
    config.pin_reset    = CAM_PIN_RESET;
    config.pin_xclk     = CAM_PIN_XCLK;
    config.pin_sccb_sda = CAM_PIN_SIOD;
    config.pin_sccb_scl = CAM_PIN_SIOC;
    config.pin_d7       = CAM_PIN_D7;
    config.pin_d6       = CAM_PIN_D6;
    config.pin_d5       = CAM_PIN_D5;
    config.pin_d4       = CAM_PIN_D4;
    config.pin_d3       = CAM_PIN_D3;
    config.pin_d2       = CAM_PIN_D2;
    config.pin_d1       = CAM_PIN_D1;
    config.pin_d0       = CAM_PIN_D0;
    config.pin_vsync    = CAM_PIN_VSYNC;
    config.pin_href     = CAM_PIN_HREF;
    config.pin_pclk     = CAM_PIN_PCLK;

    config.xclk_freq_hz  = 20000000;
    config.ledc_timer    = LEDC_TIMER_0;
    config.ledc_channel  = LEDC_CHANNEL_0;
    config.pixel_format  = PIXFORMAT_JPEG;
    config.frame_size    = FRAMESIZE_UXGA;
    config.jpeg_quality  = 10;
    config.fb_count      = 2;
    config.fb_location   = CAMERA_FB_IN_PSRAM;
    config.grab_mode     = CAMERA_GRAB_WHEN_EMPTY;
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

    sensor = esp_camera_sensor_get();
    if (!sensor) {
        ESP_LOGE(TAG, "Failed to get camera sensor");
        esp_camera_deinit();
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Sensor ID: 0x%02X", sensor->id.PID);
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

void Camera::returnFrame(camera_fb_t* fb) {
    if (fb) {
        esp_camera_fb_return(fb);
    }
}

esp_err_t Camera::initSD() {
    ESP_LOGI(TAG, "Initializing SD card (SPI mode)...");

    // 1. Safe Zero-Initialize the SPI bus config structure first
    spi_bus_config_t bus_cfg = {};
    
    // Assign only the explicitly required values
    bus_cfg.mosi_io_num = 15;
    bus_cfg.miso_io_num = 2;
    bus_cfg.sclk_io_num = 14;
    bus_cfg.quadwp_io_num = -1;
    bus_cfg.quadhd_io_num = -1;
    bus_cfg.max_transfer_sz = 4000;

    esp_err_t err = spi_bus_initialize(HSPI_HOST, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "SPI bus init failed: 0x%x", err);
        return err;
    }

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = GPIO_NUM_13;
    slot_config.host_id = HSPI_HOST;

    // 2. Safe Zero-Initialize the FAT VFS mount configuration structure
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {};
    
    // Assign only your explicit custom preferences
    mount_config.format_if_mount_failed = false;
    mount_config.max_files = 5;
    mount_config.allocation_unit_size = 16 * 1024;

    sdmmc_card_t* card;
    err = esp_vfs_fat_sdspi_mount("/sdcard", &host, &slot_config, &mount_config, &card);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "SD card mount failed: 0x%x", err);
        spi_bus_free(HSPI_HOST);
        return err;
    }

    sdMounted = true;
    ESP_LOGI(TAG, "SD card mounted successfully");
    sdmmc_card_print_info(stdout, card);
    return ESP_OK;
}


esp_err_t Camera::saveFrameToDisk(camera_fb_t* fb) {
    if (!fb) {
        ESP_LOGE(TAG, "No frame to save");
        return ESP_ERR_INVALID_ARG;
    }
    if (!sdMounted) {
        ESP_LOGE(TAG, "SD card not mounted");
        return ESP_ERR_INVALID_STATE;
    }

    FILE* f = fopen("/sdcard/img.jpg", "wb");
    if (!f) {
        ESP_LOGE(TAG, "fopen failed, errno: %d (%s)", errno, strerror(errno));
        return ESP_FAIL;
    }

    size_t written = fwrite(fb->buf, 1, fb->len, f);
    fclose(f);

    if (written != fb->len) {
        ESP_LOGE(TAG, "Write incomplete: %u of %u bytes", written, fb->len);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Saved frame to %s (%u bytes)", "/sdcard/img.jpg", fb->len);
    return ESP_OK;
}

esp_err_t Camera::recordVideoToDisk(camera_fb_t* fb, FILE *f) {
    if (!fb) {
        ESP_LOGE(TAG, "No frame to save");
        return ESP_ERR_INVALID_ARG;
    }

    if (!sdMounted) {
        ESP_LOGE(TAG, "SD card not mounted");
        return ESP_ERR_INVALID_STATE;
    }

    size_t bytesWritten = fwrite(fb->buf, 1, fb->len, f);

    if (bytesWritten < fb->len) {
        ESP_LOGE(TAG, "Fwrite failed: disk full or error");
        return ESP_ERR_INVALID_STATE;
    }
    return ESP_OK;
}

void Camera::publishROSTopicCam() {

    rcl_allocator_t allocator = rcl_get_default_allocator();
    rclc_support_t support;

    // create init_options
    RCCHECK(rclc_support_init(&support, 0, NULL, &allocator));

    // create node
    rcl_node_t node;
    RCCHECK(rclc_node_init_default(&node, "esp32_int32_publisher", "", &support));

    // create publisher
    RCCHECK(rclc_publisher_init_default(
        &publisher,
        &node,
        ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32),
        "freertos_int32_publisher"));

    // create timer
    rcl_timer_t timer;
    const unsigned int timer_timeout = 1000;
    RCCHECK(rclc_timer_init_default2(
        &timer,
        &support,
        RCL_MS_TO_NS(timer_timeout),
        &Camera::timer_callback, // Call via clear address scope
        true));

    // create executor
    rclc_executor_t executor;
    RCCHECK(rclc_executor_init(&executor, &support.context, 1, &allocator));
    RCCHECK(rclc_executor_add_timer(&executor, &timer));

    msg.data = 0;

    while(1){
        rclc_executor_spin_some(&executor, RCL_MS_TO_NS(100));
        std::this_thread::sleep_for(std::chrono::microseconds(10000));
    }

    // free resources
    RCCHECK(rcl_publisher_fini(&publisher, &node));
    RCCHECK(rcl_node_fini(&node));

    vTaskDelete(NULL);
}


void Camera::timer_callback(rcl_timer_t * timer, int64_t last_call_time)
{
	RCLC_UNUSED(last_call_time);
	if (timer != NULL) {
		RCSOFTCHECK(rcl_publish(&publisher, &msg, NULL));
		msg.data++;
	}
}

bool Camera::isInitialized() const {
    return initialized;
}

void Camera::on() {
    if (!initialized) init();
}

void Camera::off() {
    if (initialized) {
        esp_camera_deinit();
        initialized = false;
    }
}