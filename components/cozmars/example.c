/* Example: proper hardware initialization order for cozmars-head.
 *
 * This file is NOT part of the build (not listed in CMakeLists.txt) - it is
 * documentation. Copy the init sequence below into your own app_main().
 *
 * Note: `tft` is a C++ object defined by lgfx.hpp. If you use the display,
 * put this sequence in a C++ source file.
 */
#include <assert.h>
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern "C" {
#include "wifi.h"
#include "i2c_bus.h"
#include "codec.h"
#include "sub_ctl.h"
#include "camera.h"
}
#include "lgfx.hpp"

void app_main(void)
{
    /* 1. Wi-Fi: connect to stored network, or open config hotspot */
    ESP_ERROR_CHECK(wifi_init());

    /* 2. Display (ST7789 over SPI, via LovyanGFX) - init() returns bool */
    assert(tft.init() && "tft init failed");

    /* 3. I2C buses (must be initialized before codec, sub and camera) */
    ESP_ERROR_CHECK(i2c_buses_init());

    /* 4. Audio codec (ES8311 out / ES7210 in) */
    ESP_ERROR_CHECK(codec_init(CODEC_MIC1 | CODEC_MIC2 | CODEC_REF));

    /* 5. Sub-processor link (motors, servos, WLED) */
    ESP_ERROR_CHECK(subc_init());

    /* 6. Camera (DVP + GC0308, requires I2C bus) */
    ESP_ERROR_CHECK(cam_init());

    ESP_LOGI("cozmars", "initialized");
    vTaskDelete(NULL);
}
