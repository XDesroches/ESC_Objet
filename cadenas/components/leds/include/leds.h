#pragma once
#include "driver/gpio.h"
#ifdef __cplusplus
extern "C" {
#endif
void led_init(gpio_num_t pin);
void led_on(gpio_num_t pin);
void led_off(gpio_num_t pin);
void led_toggle(gpio_num_t pin);
#ifdef __cplusplus
}
#endif
