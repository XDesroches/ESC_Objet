idf_component_register(SRCS "main.c"
                       INCLUDE_DIRS "."
                       REQUIRES lcd1602_i2c joystick leds buzzer esp_timer)
