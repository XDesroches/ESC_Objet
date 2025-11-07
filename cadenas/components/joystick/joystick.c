#include "joystick.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include <math.h>

static joystick_config_t s_cfg;
static int s_btn_prev = 1;
static uint64_t s_btn_last_change = 0;

static inline uint64_t now_ms(void){
    int64_t us = esp_timer_get_time();
    return (uint64_t)(us / 1000ULL);
}

void joystick_init(const joystick_config_t *cfg){
    s_cfg = *cfg;
    adc1_config_width(cfg->width);
    adc1_config_channel_atten(cfg->chan_x, cfg->atten);
    adc1_config_channel_atten(cfg->chan_y, cfg->atten);

    gpio_config_t ib = {
        .pin_bit_mask = (1ULL << cfg->btn_gpio),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = cfg->btn_pullup ? 1 : 0,
        .pull_down_en = 0,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&ib);
    s_btn_prev = gpio_get_level(cfg->btn_gpio);
    s_btn_last_change = now_ms();
}

void joystick_calibrate(joystick_calib_t *c, int ms){
    int64_t sx=0, sy=0; int n=0;
    uint64_t t0 = now_ms();
    while(now_ms() - t0 < (uint64_t)ms){
        sx += adc1_get_raw(s_cfg.chan_x);
        sy += adc1_get_raw(s_cfg.chan_y);
        n++; vTaskDelay(pdMS_TO_TICKS(2));
    }
    if (n <= 0) n = 1;
    c->cx = (int)(sx/n);
    c->cy = (int)(sy/n);
}

void joystick_read_raw(int *x, int *y){
    if (x) *x = adc1_get_raw(s_cfg.chan_x);
    if (y) *y = adc1_get_raw(s_cfg.chan_y);
}

float joystick_angle_deg(int x, int y, const joystick_calib_t *c){
    float fx = (float)(x - c->cx) / 2048.0f;
    float fy = (float)(c->cy - y) / 2048.0f;
    float deg = atan2f(fy, fx) * (180.0f / (float)M_PI);
    if (deg < 0) deg += 360.0f;
    return deg;
}

bool joystick_button_edge_falling(void){
    int level = gpio_get_level(s_cfg.btn_gpio);
    uint64_t t = now_ms();
    if (level != s_btn_prev){
        s_btn_prev = level;
        s_btn_last_change = t;
    }
    if (level == 0 && (t - s_btn_last_change) > 40){
        while (gpio_get_level(s_cfg.btn_gpio) == 0) vTaskDelay(pdMS_TO_TICKS(5));
        return true;
    }
    return false;
}
