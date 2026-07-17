#include <stdio.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/spi_master.h"
#include "driver/ledc.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_st7789.h"
#include "esp_heap_caps.h"
#include "head_config.h"
#include "lcd.h"
#include "util.h"

static const char *TAG = "lcd";

#define LCD_HOST          SPI2_HOST
#define LCD_H_RES         240
#define LCD_V_RES         135
#define LCD_CMD_BITS      8
#define LCD_PARAM_BITS    8

#define LCD_BK_SPEED_MODE      LEDC_LOW_SPEED_MODE
#define LCD_BK_TIMER           LEDC_TIMER_0
#define LCD_BK_CHANNEL         LEDC_CHANNEL_0
#define LCD_BK_DUTY_RES       LEDC_TIMER_8_BIT
#define LCD_BK_DUTY_MAX       255
#define LCD_BK_FREQ_HZ        5000

static esp_lcd_panel_handle_t panel_handle = NULL;

static void init_backlight(void)
{
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LCD_BK_SPEED_MODE,
        .duty_resolution  = LCD_BK_DUTY_RES,
        .timer_num        = LCD_BK_TIMER,
        .freq_hz          = LCD_BK_FREQ_HZ,
        .clk_cfg          = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));
    ledc_channel_config_t ledc_channel = {
        .gpio_num       = LCD_BK_IO,
        .speed_mode     = LCD_BK_SPEED_MODE,
        .channel        = LCD_BK_CHANNEL,
        .timer_sel      = LCD_BK_TIMER,
        .duty           = 0,
        .hpoint         = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}

void lcd_init(void)
{
    init_backlight();

    ESP_LOGI(TAG, "Initialize SPI bus");
    spi_bus_config_t buscfg = {
        .sclk_io_num = LCD_SPI_SCLK_IO,
        .mosi_io_num = LCD_SPI_MOSI_IO,
        .miso_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_H_RES * 80 * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

    ESP_LOGI(TAG, "Install panel IO");
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = LCD_DC_IO,
        .cs_gpio_num = LCD_CS_IO,
        .pclk_hz = 20 * 1000 * 1000,
        .lcd_cmd_bits = LCD_CMD_BITS,
        .lcd_param_bits = LCD_PARAM_BITS,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(LCD_HOST, &io_config, &io_handle));

    ESP_LOGI(TAG, "Install ST7789 panel driver");
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = LCD_RST_IO,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, false));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
}

void lcd_set_backlight(int pct)
{
    pct = clamp_int(pct, 0, 100);
    ESP_ERROR_CHECK(ledc_set_duty(LCD_BK_SPEED_MODE, LCD_BK_CHANNEL, (pct * LCD_BK_DUTY_MAX) / 100));
    ESP_ERROR_CHECK(ledc_update_duty(LCD_BK_SPEED_MODE, LCD_BK_CHANNEL));
}
