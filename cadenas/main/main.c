
#include "esp_random.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "buzzer.h"
#include "joystick.h"
#include "lcd1602_i2c.h"
#include "leds.h"

// ---------------- Pins ----------------
#define PIN_LED_VERTE GPIO_NUM_18
#define PIN_LED_ROUGE GPIO_NUM_19
#define PIN_BUZZER GPIO_NUM_25
#define PIN_BTN_SW GPIO_NUM_27 // handled by joystick component

// ---------------- Game config ----------------
#define SEQ_LEN 3
#define MAX_LIVES 3

// Tolerances / beeps
#define TOL_OK_DEG 15.0f // validate guess if |error| <= 15°
#define NEAR2_DEG 20.0f  // very close -> fast beep
#define NEAR1_DEG 40.0f  // close -> medium beep
#define BEEP_FAST_MS 80
#define BEEP_MED_MS 250

#define LCD_BACKPACK_VARIANT 2

static inline uint64_t now_ms(void) {
    return esp_timer_get_time() / 1000ULL;
}

// ---------------- UI ----------------
static void ui_step(int idx, int target_deg, int lives) {
    lcd1602_clear();

    // Line1: "Etape X/3 Vie:Y"
    lcd1602_set_cursor(0, 0);
    lcd1602_print("Etape ");
    int n = idx + 1;
    if (n < 1)
        n = 1;
    if (n > 3)
        n = 3;
    char step[4] = {(char)('0' + n), '/', '3', '\0'};
    lcd1602_print(step);
    lcd1602_print(" Vie:");
    char lv[3];
    lv[0] = (char)('0' + (lives % 10));
    lv[1] = ' ';
    lv[2] = '\0';
    lcd1602_print(lv);

    // Line2: "Cible: xxx°"
    int td = target_deg % 360;
    if (td < 0)
        td += 360;
    lcd1602_set_cursor(0, 1);
    lcd1602_print("Cible: ");
    char num[4];
    num[0] = (td >= 100) ? ('0' + (td / 100) % 10) : ' ';
    num[1] = (td >= 10) ? ('0' + (td / 10) % 10) : ' ';
    num[2] = ('0' + (td % 10));
    num[3] = '\0';
    lcd1602_print(num);
    char deg[2] = {(char)0xDF, '\0'};
    lcd1602_print(deg);
}

static void ui_wrong_hint(void) {
    lcd1602_set_cursor(0, 1);
    lcd1602_print("Mauvaise dir   ");
}

static void ui_victoire(void) {
    lcd1602_clear();
    lcd1602_set_cursor(0, 0);
    lcd1602_print("Victoire!");
    lcd1602_set_cursor(0, 1);
    lcd1602_print("Deverrouille");
}

static void ui_defaite(void) {
    lcd1602_clear();
    lcd1602_set_cursor(0, 0);
    lcd1602_print("Defaite :(");
    lcd1602_set_cursor(0, 1);
    lcd1602_print("Reessaie");
}

// ---------------- Buzzer helpers ----------------
static void win_anim(void) {
    led_on(PIN_LED_VERTE);
    buzzer_tone(523, 18, 100);
    led_off(PIN_LED_VERTE);
    led_on(PIN_LED_VERTE);
    buzzer_tone(659, 18, 100);
    led_off(PIN_LED_VERTE);
    led_on(PIN_LED_VERTE);
    buzzer_tone(784, 18, 120);
    led_off(PIN_LED_VERTE);
    led_on(PIN_LED_VERTE);
    buzzer_tone(1047, 18, 180);
    led_off(PIN_LED_VERTE);
    buzzer_restore_base();
}

static void lose_anim(void) {
    led_on(PIN_LED_ROUGE);
    buzzer_tone(800, 16, 140);
    led_off(PIN_LED_ROUGE);
    vTaskDelay(pdMS_TO_TICKS(60));
    led_on(PIN_LED_ROUGE);
    buzzer_tone(600, 16, 160);
    led_off(PIN_LED_ROUGE);
    buzzer_restore_base();
}

// ---------------- Math helpers ----------------
static float ang_err_abs(float a, float target) {
    float d = fabsf(a - target);
    if (d > 180.0f)
        d = 360.0f - d;
    return d;
}

// Random targets with separation
static float TARGETS_DEG[SEQ_LEN];
static inline float rand_angle_deg(void) {
    return (float)(esp_random() % 360);
}
static void generate_targets(void) {
    const float MIN_SEP = 30.0f;
    for (int i = 0; i < SEQ_LEN; i++) {
        int tries = 0;
        while (1) {
            float a = rand_angle_deg();
            int ok = 1;
            for (int j = 0; j < i; j++) {
                float d = fabsf(a - TARGETS_DEG[j]);
                if (d > 180.0f)
                    d = 360.0f - d;
                if (d < MIN_SEP) {
                    ok = 0;
                    break;
                }
            }
            if (ok || tries++ > 64) {
                TARGETS_DEG[i] = a;
                break;
            }
        }
    }
}

void app_main(void) {
    // LEDs
    led_init(PIN_LED_VERTE);
    led_init(PIN_LED_ROUGE);

    // LCD (I2C0, 0x27)
    lcd1602_i2c_config_t lcfg = {.port = I2C_NUM_0,
                                 .sda_io = GPIO_NUM_21,
                                 .scl_io = GPIO_NUM_22,
                                 .clk_speed_hz = 50000,
                                 .i2c_addr = 0x27,
                                 .enable_backlight = true};
    lcd1602_i2c_init(&lcfg);
    lcd1602_clear();
    lcd1602_set_cursor(0, 0);
    lcd1602_print("Cadenas 360deg");
    lcd1602_set_cursor(0, 1);
    lcd1602_print("3 cibles alea");
    vTaskDelay(pdMS_TO_TICKS(800));

    // Joystick
    joystick_config_t jcfg = {.chan_x = ADC1_CHANNEL_6,
                              .chan_y = ADC1_CHANNEL_7,
                              .width = ADC_WIDTH_BIT_12,
                              .atten = ADC_ATTEN_DB_12,
                              .btn_gpio = PIN_BTN_SW,
                              .btn_pullup = true};
    joystick_init(&jcfg);
    joystick_calib_t calib;
    joystick_calibrate(&calib, 800);

    // Buzzer
    buzzer_config_t bcfg = {.pin = PIN_BUZZER,
                            .speed_mode = LEDC_LOW_SPEED_MODE,
                            .timer_num = LEDC_TIMER_0,
                            .channel = LEDC_CHANNEL_0,
                            .duty_resolution = LEDC_TIMER_8_BIT,
                            .base_freq_hz = 4000,
                            .default_duty = 20};
    buzzer_init(&bcfg);

    // Targets
    generate_targets();
    int step = 0, correct = 0, submitted = 0;
    int lives = MAX_LIVES;
    ui_step(step, (int)lroundf(TARGETS_DEG[step]), lives);

    // Beep state
    uint64_t next_toggle = now_ms();
    int beep_on = 0;
    int period_ms = 0;

    while (1) {
        int rx, ry;
        joystick_read_raw(&rx, &ry);
        float ang = joystick_angle_deg(rx, ry, &calib);
        float target = TARGETS_DEG[step];
        float err = ang_err_abs(ang, target);

        // proximity beep
        int new_period = (err <= NEAR2_DEG) ? BEEP_FAST_MS : (err <= NEAR1_DEG ? BEEP_MED_MS : 0);
        if (new_period != period_ms) {
            period_ms = new_period;
            beep_on = 0;
            buzzer_off();
            if (period_ms > 0)
                next_toggle = now_ms() + period_ms;
        }
        if (period_ms > 0) {
            uint64_t t = now_ms();
            if (t >= next_toggle) {
                next_toggle = t + period_ms;
                beep_on = !beep_on;
                if (beep_on)
                    buzzer_on();
                else
                    buzzer_off();
            }
        } else {
            buzzer_off();
        }

        // submit
        if (joystick_button_edge_falling()) {
            submitted++;
            if (err <= TOL_OK_DEG) {
                led_on(PIN_LED_VERTE);
                vTaskDelay(pdMS_TO_TICKS(120));
                led_off(PIN_LED_VERTE);
                correct++;
                if (step < SEQ_LEN - 1) {
                    step++;
                    ui_step(step, (int)lroundf(TARGETS_DEG[step]), lives);
                } else {
                    ui_victoire();
                    win_anim();
                    generate_targets();
                    step = 0;
                    correct = 0;
                    submitted = 0;
                    lives = MAX_LIVES;
                    ui_step(step, (int)lroundf(TARGETS_DEG[step]), lives);
                }
            } else {
                if (lives > 0)
                    lives--;
                led_on(PIN_LED_ROUGE);
                ui_wrong_hint();
                vTaskDelay(pdMS_TO_TICKS(200));
                led_off(PIN_LED_ROUGE);
                if (lives <= 0) {
                    ui_defaite();
                    lose_anim();
                    generate_targets();
                    step = 0;
                    correct = 0;
                    submitted = 0;
                    lives = MAX_LIVES;
                    ui_step(step, (int)lroundf(TARGETS_DEG[step]), lives);
                } else {
                    step = 0;
                    correct = 0;
                    submitted = 0;
                    ui_step(step, (int)lroundf(TARGETS_DEG[step]), lives);
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
