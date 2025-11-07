#pragma once
#include "driver/ledc.h"
#include "driver/gpio.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    gpio_num_t pin;
    ledc_mode_t speed_mode;
    ledc_timer_t timer_num;
    ledc_channel_t channel;
    ledc_timer_bit_t duty_resolution;
    uint32_t base_freq_hz;
    int default_duty;
} buzzer_config_t;

esp_err_t buzzer_init(const buzzer_config_t *cfg);
void      buzzer_set_volume(int duty);
void      buzzer_on(void);
void      buzzer_off(void);
uint32_t  buzzer_set_freq(uint32_t hz);
void      buzzer_restore_base(void);
void      buzzer_tone(uint32_t hz, int duty, int ms);

#ifdef __cplusplus
}
#endif
