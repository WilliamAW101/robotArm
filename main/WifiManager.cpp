#include <WifiManager.h>
#include "esp_log.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "WifiManager";

WifiManager::WifiManager(const char userName, const char password) {
    // empty for now
}