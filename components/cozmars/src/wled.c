#include "wled.h"

esp_err_t wled_set_color(wled_color_t c)
{
    return subc_send_msg(&(sub_msg_t){
        .type = SUB_MSG_WR_LED,
        .wled_cmd.type = WLED_CMD_SET_COLOR,
        .wled_cmd.color_arg = c,
    });
}

esp_err_t wled_off(void)
{
    return wled_set_color((wled_color_t){0});
}

esp_err_t wled_blink2(wled_color_t c, uint32_t dur1, wled_color_t c2, uint32_t dur2, uint32_t repeat)
{
    return subc_send_msg(&(sub_msg_t){
        .type = SUB_MSG_WR_LED,
        .wled_cmd.type = WLED_CMD_BLINK,
        .wled_cmd.blink_arg = {
            .color1 = c,
            .dur1 = dur1,
            .color2 = c2,
            .dur2 = dur2,
            .repeat = repeat,
        },
    });
}

esp_err_t wled_blink1(wled_color_t c, uint32_t on_dur, uint32_t off_dur, uint32_t repeat)
{
    return wled_blink2(c, on_dur, (wled_color_t){0}, off_dur, repeat);
}

esp_err_t wled_blink(wled_color_t c, uint32_t on_dur, uint32_t off_dur)
{
    return wled_blink2(c, on_dur, (wled_color_t){0}, off_dur, 0);
}

esp_err_t wled_fade2(wled_color_t c, uint32_t fade_up_dur, wled_color_t c2, uint32_t fade_down_dur, uint32_t repeat)
{
    return subc_send_msg(&(sub_msg_t){
        .type = SUB_MSG_WR_LED,
        .wled_cmd.type = WLED_CMD_FADE,
        .wled_cmd.fade_arg = {
            .color1 = c,
            .fade_up_dur = fade_up_dur,
            .color2 = c2,
            .fade_down_dur = fade_down_dur,
            .repeat = repeat,
        },
    });
}

esp_err_t wled_fade1(wled_color_t c, uint32_t fade_up_dur, uint32_t fade_down_dur, uint32_t repeat)
{
    return wled_fade2((wled_color_t){0}, fade_up_dur, c, fade_down_dur, repeat);
}

esp_err_t wled_fade(wled_color_t c, uint32_t fade_up_dur, uint32_t fade_down_dur)
{
    return wled_fade2((wled_color_t){0}, fade_up_dur, c, fade_down_dur, 0);
}
