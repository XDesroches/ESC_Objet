#pragma once
#include "driver/i2c.h"
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    i2c_port_t port;
    gpio_num_t sda_io;
    gpio_num_t scl_io;
    uint32_t   clk_speed_hz;
    uint8_t    i2c_addr;
    bool       enable_backlight;
} lcd1602_i2c_config_t;
esp_err_t lcd1602_i2c_init(const lcd1602_i2c_config_t *cfg);
void      lcd1602_clear(void);
void      lcd1602_home(void);
void      lcd1602_set_cursor(uint8_t col, uint8_t row);
void      lcd1602_print(const char *s);
void      lcd1602_backlight(bool on);
#ifdef __cplusplus
}
#endif
