#include "leds.h"

void led_init(gpio_num_t pin){
    gpio_config_t o = {0};
    o.pin_bit_mask = (1ULL << pin);
    o.mode = GPIO_MODE_OUTPUT;
    gpio_config(&o);
    gpio_set_level(pin, 0);
}

void led_on(gpio_num_t pin){ gpio_set_level(pin, 1); }

void led_off(gpio_num_t pin){ gpio_set_level(pin, 0); }

void led_toggle(gpio_num_t pin){ gpio_set_level(pin, !gpio_get_level(pin)); }
