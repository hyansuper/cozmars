/* Display two cyan rounded rectangular eyes (Cozmo style) on the ST7789 LCD.
 * When blinking, a horizontal line is drawn across the whole screen. */
#include <stdio.h>
#include <assert.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_random.h"
#include "lgfx.hpp"

static const char *TAG = "display_eye";

/* Landscape orientation (rotation 1): 240x135 */
#define SCREEN_W   240
#define SCREEN_H   135

#define EYE_W      64    /* eye width and height in px */
#define EYE_RADIUS 20    /* corner radius, > EYE_W/2 makes the ends fully round */

#define EYE_GAP    36    /* gap between the eyes */

/* eyes are centered as a pair */
#define EYE1_X     ((SCREEN_W - 2 * EYE_W - EYE_GAP) / 2)
#define EYE2_X     (EYE1_X + EYE_W + EYE_GAP)

#define COLOR_CYAN 0x07ff

static void draw_eyes(void)
{
    int y = (SCREEN_H - EYE_W) / 2;
    tft.fillRoundRect(EYE1_X, y, EYE_W, EYE_W, EYE_RADIUS, COLOR_CYAN);
    tft.fillRoundRect(EYE2_X, y, EYE_W, EYE_W, EYE_RADIUS, COLOR_CYAN);
}

static void erase_eyes(void)
{
    int y = (SCREEN_H - EYE_W) / 2;
    tft.fillRect(EYE1_X, y, EYE_W, EYE_W, TFT_BLACK);
    tft.fillRect(EYE2_X, y, EYE_W, EYE_W, TFT_BLACK);
}

extern "C" void app_main(void)
{
    assert(tft.init() && "tft init failed");

    tft.fillScreen(TFT_BLACK);
    draw_eyes();

    while (true) {
        /* keep eyes open for a random 2-5 s, then blink briefly */
        uint32_t awake_ms = 2000 + esp_random() % 3000;
        vTaskDelay(pdMS_TO_TICKS(awake_ms));

        erase_eyes();
        /* blink line spans ~90% of the screen width, centered */
        tft.drawFastHLine(SCREEN_W / 10, SCREEN_H / 2,
                          SCREEN_W - 2 * (SCREEN_W / 10), COLOR_CYAN);
        vTaskDelay(pdMS_TO_TICKS(120));
        tft.fillScreen(TFT_BLACK);
        draw_eyes();
    }
}
