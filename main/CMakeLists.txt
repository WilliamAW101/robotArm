idf_component_register(SRCS 
"main.cpp"
"LED.cpp"
"Camera.cpp"
"WifiManager.cpp" 
INCLUDE_DIRS "."
REQUIRES esp32-camera
REQUIRES esp_http_server
REQUIRES esp_timer
REQUIRES esp_http_client
REQUIRES esp_wifi)