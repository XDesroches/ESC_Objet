#include "lcd1602_i2c.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_rom_sys.h"

#define PIN_RS     0x01
#define PIN_EN     0x04
#define PIN_BL     0x08

static i2c_port_t s_port;
static uint8_t    s_addr;
static bool       s_backlight = true;

static esp_err_t i2c_write_byte(uint8_t data)
{
    uint8_t out = (uint8_t)(data | (s_backlight ? PIN_BL : 0));
    return i2c_master_write_to_device(s_port, s_addr, &out, 1, pdMS_TO_TICKS(100));
}

static void lcd_pulse_enable(uint8_t data)
{
    i2c_write_byte((uint8_t)(data | PIN_EN));
    esp_rom_delay_us(2);
    i2c_write_byte((uint8_t)(data & (uint8_t)~PIN_EN));
    esp_rom_delay_us(100);
}

static void lcd_write4bits(uint8_t value_hi, bool rs)
{
    uint8_t data = (uint8_t)(value_hi & 0xF0);
    if (rs) data |= PIN_RS;
    i2c_write_byte(data);
    lcd_pulse_enable(data);
}

static void lcd_send(uint8_t value, bool rs)
{
    lcd_write4bits((uint8_t)(value & 0xF0), rs);
    lcd_write4bits((uint8_t)((value << 4) & 0xF0), rs);
}

static inline void lcd_command(uint8_t cmd)
{
    lcd_send(cmd, false);
    if (cmd == 0x01 || cmd == 0x02) vTaskDelay(pdMS_TO_TICKS(5));
}

static inline void lcd_data(uint8_t d) { lcd_send(d, true); }

static void lcd_hw_init(void)
{
    vTaskDelay(pdMS_TO_TICKS(50));
    lcd_write4bits(0x30,false); vTaskDelay(pdMS_TO_TICKS(5));
    lcd_write4bits(0x30,false); vTaskDelay(pdMS_TO_TICKS(5));
    lcd_write4bits(0x30,false); vTaskDelay(pdMS_TO_TICKS(5));
    lcd_write4bits(0x20,false); vTaskDelay(pdMS_TO_TICKS(5));

    lcd_command(0x28); vTaskDelay(pdMS_TO_TICKS(5));
    lcd_command(0x0C); vTaskDelay(pdMS_TO_TICKS(5));
    lcd_command(0x01); vTaskDelay(pdMS_TO_TICKS(5));
    lcd_command(0x06); vTaskDelay(pdMS_TO_TICKS(5));
}

// ================= API du composant =================
esp_err_t lcd1602_i2c_init(const lcd1602_i2c_config_t *cfg)
{
    if (!cfg) return ESP_ERR_INVALID_ARG;
    s_port      = cfg->port;
    s_addr      = cfg->i2c_addr;
    s_backlight = cfg->enable_backlight;

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = cfg->sda_io,
        .scl_io_num = cfg->scl_io,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = cfg->clk_speed_hz ? cfg->clk_speed_hz : 50000,
    };
    ESP_ERROR_CHECK(i2c_param_config(s_port, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(s_port, conf.mode, 0, 0, 0));

    i2c_write_byte(0x00);

    lcd_hw_init();
    lcd1602_backlight(s_backlight);
    return ESP_OK;
}

void lcd1602_clear(void) { lcd_command(0x01); vTaskDelay(pdMS_TO_TICKS(5)); }

void lcd1602_home(void)  { lcd_command(0x02); vTaskDelay(pdMS_TO_TICKS(5)); }

void lcd1602_set_cursor(uint8_t col, uint8_t row)
{
    static const uint8_t row_offsets[] = {0x00, 0x40, 0x14, 0x54};
    if (row > 1) row = 1;
    lcd_command((uint8_t)(0x80 | (col + row_offsets[row])));
}

void lcd1602_print(const char *s)
{
    if (!s) return;
    while (*s) lcd_data((uint8_t)*s++);
}

void lcd1602_backlight(bool on)
{
    s_backlight = on;
    i2c_write_byte(0x00);
}
