#include "sub_ctl.h"

esp_err_t wled_set_color(wled_color_t c);
esp_err_t wled_off();

esp_err_t wled_blink(wled_color_t c, uint32_t on_dur, uint32_t off_dur);
esp_err_t wled_blink1(wled_color_t c, uint32_t on_dur, uint32_t off_dur, uint32_t repeat);
esp_err_t wled_blink2(wled_color_t c, uint32_t dur1, wled_color_t c2, uint32_t dur2, uint32_t repeat);

esp_err_t wled_fade(wled_color_t c, uint32_t fade_up_dur, uint32_t fade_down_dur);
esp_err_t wled_fade1(wled_color_t c, uint32_t fade_up_dur, uint32_t fade_down_dur, uint32_t repeat);
esp_err_t wled_fade2(wled_color_t c, uint32_t fade_up_dur, wled_color_t c2, uint32_t fade_down_dur, uint32_t repeat);