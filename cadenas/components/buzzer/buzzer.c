#include "buzzer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static buzzer_config_t s_cfg;
static int s_duty;

esp_err_t buzzer_init(const buzzer_config_t *cfg){
    s_cfg = *cfg;
    s_duty = cfg->default_duty;

    ledc_timer_config_t tcfg = {
        .speed_mode = cfg->speed_mode,
        .duty_resolution = cfg->duty_resolution,
        .timer_num = cfg->timer_num,
        .freq_hz = cfg->base_freq_hz,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&tcfg));

    ledc_channel_config_t ccfg = {
        .gpio_num = cfg->pin,
        .speed_mode = cfg->speed_mode,
        .channel = cfg->channel,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = cfg->timer_num,
        .duty = 0,
        .hpoint = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ccfg));
    return ESP_OK;
}

void buzzer_set_volume(int duty){ s_duty = duty; }

void buzzer_on(void){
    ledc_set_duty(s_cfg.speed_mode, s_cfg.channel, s_duty);
    ledc_update_duty(s_cfg.speed_mode, s_cfg.channel);
}

void buzzer_off(void){
    ledc_set_duty(s_cfg.speed_mode, s_cfg.channel, 0);
    ledc_update_duty(s_cfg.speed_mode, s_cfg.channel);
}

uint32_t buzzer_set_freq(uint32_t hz){
    return ledc_set_freq(s_cfg.speed_mode, s_cfg.timer_num, hz);
}

void buzzer_restore_base(void){
    ledc_set_freq(s_cfg.speed_mode, s_cfg.timer_num, s_cfg.base_freq_hz);
}

void buzzer_tone(uint32_t hz, int duty, int ms){
    uint32_t real = buzzer_set_freq(hz);
    if (real == 0) {
        buzzer_set_freq(1000);
    }
    ledc_set_duty(s_cfg.speed_mode, s_cfg.channel, duty);
    ledc_update_duty(s_cfg.speed_mode, s_cfg.channel);
    vTaskDelay(pdMS_TO_TICKS(ms));
    buzzer_off();
    vTaskDelay(pdMS_TO_TICKS(20));
}
