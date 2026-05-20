idf_component_register(SRCS "main.cpp" "Camera.cpp" "LED.cpp"
                        INCLUDE_DIRS "."
                        REQUIRES fatfs sdmmc esp_driver_sdmmc esp_driver_spi)