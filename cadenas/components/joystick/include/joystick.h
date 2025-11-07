#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "driver/gpio.h"
#include "driver/adc.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    adc1_channel_t chan_x;
    adc1_channel_t chan_y;
    adc_bits_width_t width;
    adc_atten_t atten;
    gpio_num_t btn_gpio;
    bool btn_pullup;
} joystick_config_t;

typedef struct {
    int cx;
    int cy;
} joystick_calib_t;

void joystick_init(const joystick_config_t *cfg);
void joystick_calibrate(joystick_calib_t *c, int ms);
void joystick_read_raw(int *x, int *y);
float joystick_angle_deg(int x, int y, const joystick_calib_t *c);
bool joystick_button_edge_falling(void);

#ifdef __cplusplus
}
#endif
