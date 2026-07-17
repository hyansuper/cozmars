#pragma once

#include <LovyanGFX.hpp>
#include "head_config.h"

// Backlight brightness set by percentage (0-100) instead of 0-255.
class Light_PWM_Percent : public lgfx::Light_PWM
{
public:
    void setBrightness(std::uint8_t brightness) override
    {
        if (brightness > 100) brightness = 100;
        lgfx::Light_PWM::setBrightness(brightness * 255 / 100);
    }
};

// LovyanGFX device configuration, modeled on
// LovyanGFX/src/lgfx_user/LGFX_ESP32_S3_Touch_LCD_2.h (no touch).
// ST7789 240x135 (SPI) | ESP32-S3
class LGFX : public lgfx::LGFX_Device {
    lgfx::Panel_ST7789 panel_instance_;
    lgfx::Bus_SPI      bus_instance_;
    Light_PWM_Percent  light_instance_;

public:
    LGFX(void) {
        {
            auto cfg = bus_instance_.config();
            cfg.spi_host = SPI2_HOST;
            cfg.spi_mode = 0;
            cfg.freq_write = 60000000;
            cfg.freq_read = 16000000;
            cfg.spi_3wire = false;
            cfg.use_lock = true;
            cfg.dma_channel = SPI_DMA_CH_AUTO;
            cfg.pin_sclk = LCD_SPI_SCLK_IO;
            cfg.pin_mosi = LCD_SPI_MOSI_IO;
            cfg.pin_miso = -1;
            cfg.pin_dc = LCD_DC_IO;
            bus_instance_.config(cfg);
            panel_instance_.setBus(&bus_instance_);
        }

        {
            auto cfg = panel_instance_.config();
            cfg.pin_cs = LCD_CS_IO;
            cfg.pin_rst = LCD_RST_IO;
            cfg.pin_busy = -1;
            cfg.memory_width = 240;
            cfg.memory_height = 135;
            cfg.panel_width = 135;
            cfg.panel_height = 240;
            cfg.offset_x = 52;
            cfg.offset_y = 40;
            cfg.offset_rotation = 1;
            cfg.readable = false;
            cfg.invert = true;
            cfg.rgb_order = false;
            cfg.bus_shared = false;
            panel_instance_.config(cfg);
        }

        {
            auto cfg = light_instance_.config();
            cfg.pin_bl = LCD_BK_IO;
            cfg.invert = LCD_BK_INVERT;
            cfg.freq = 5000;
            cfg.pwm_channel = 0;
            light_instance_.config(cfg);
            panel_instance_.setLight(&light_instance_);
        }

        setPanel(&panel_instance_);
        // setColorDepth(16);
    }
};

extern LGFX tft;

