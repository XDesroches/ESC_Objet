idf_component_register(SRCS "main.c"
                       INCLUDE_DIRS "."
                       REQUIRES lcd1602_i2c joystick leds buzzer esp_timer esp_http_client esp_wifi nvs_flash)
