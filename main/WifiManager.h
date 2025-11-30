#ifndef WIFIMANAGER_H
#define WIFIMANAGER_H

#include "esp_err.h"
#include <string>

class WifiManager {
    private:
        std::string userName;
        std::string password;
    public:
        WifiManager(const char userName, const char password);

        esp_err_t init();
        esp_err_t connect();
};

#endif