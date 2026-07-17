/* Sub board control: test WLED, motors, lift and head servos, while a
 * background task periodically reads the sensors and prints them. */
#include <unistd.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "i2c_bus.h"
#include "sub_ctl.h"
#include "motors.h"
#include "servo.h"
#include "wled.h"

/* Head servo mechanical limits */
#define HEAD_ANGLE_MIN  (-20)  /* degrees */
#define HEAD_ANGLE_MAX  (45)   /* degrees */

#define LIFT_HEIGHT_LOW  (0)   /* % */
#define LIFT_HEIGHT_HIGH (90)  /* % */

#define THROTTLE (50)          /* % */

static const char *TAG = "sub_board_control";

static void wait_ms(uint32_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}

static const char *battery_str(battery_state_t s)
{
    switch (s) {
    case BATTERY_TOO_LOW:  return "too low";
    case BATTERY_LVL_1:    return "1 (low)";
    case BATTERY_LVL_2:    return "2";
    case BATTERY_LVL_3:    return "3";
    case BATTERY_LVL_4:    return "4";
    case BATTERY_LVL_5:    return "5 (high)";
    case BATTERY_CHARGING: return "charging";
    case BATTERY_STANDBY:  return "standby (on dock)";
    default:               return "unknown";
    }
}

static void sensor_task(void *arg)
{
    sub_state_resp_t state;
    while (1) {
        if (subc_read_state(&state) == ESP_OK) {
            uint8_t d = state.hc165_data;
            printf("battery: %s | cliff IR: fL=%d fR=%d rL=%d rR=%d | enc: L=%d R=%d | touch=%d | idle: head=%d lift=%d led=%d motors=%d\n",
                   battery_str(state.battery_state),
                   !!(d & HC165_FL_IR_MASK), !!(d & HC165_FR_IR_MASK),
                   !!(d & HC165_RL_IR_MASK), !!(d & HC165_RR_IR_MASK),
                   !!(d & HC165_LM_ENC_MASK), !!(d & HC165_RM_ENC_MASK),
                   !!(d & HC165_TOUCH_MASK),
                   !!(state.idle_flags & IDLE_HEAD_MASK),
                   !!(state.idle_flags & IDLE_LIFT_MASK),
                   !!(state.idle_flags & IDLE_WLED_MASK),
                   !!(state.idle_flags & IDLE_MOTORS_MASK));
        } else {
            ESP_LOGE(TAG, "failed to read state from sub board");
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(void)
{
    ESP_ERROR_CHECK(i2c_buses_init());
    ESP_ERROR_CHECK(subc_init());
    ESP_ERROR_CHECK(subc_check_version());

    xTaskCreate(sensor_task, "sensor", 4096, NULL, 5, NULL);

    /* --- WLED: blink and fade, 3 repeats each --- */
    wled_color_t red   = {.r = 0xff};
    wled_color_t green = {.g = 0xff};
    wled_color_t blue = {.b = 0xff};

    ESP_ERROR_CHECK(wled_set_color(red));
    wait_ms(500);
    ESP_ERROR_CHECK(wled_blink1(green, 250, 250, 3));
    wait_ms(3 * (250 + 250));
    ESP_ERROR_CHECK(wled_fade1(blue, 500, 500, 3));
    wait_ms(3 * (500 + 500));

    /* --- Motors: forward/backward 5 cm at 50% throttle --- */
    ESP_LOGI(TAG, "forward");
    ESP_ERROR_CHECK(motors_go_dist(THROTTLE, 50)); /* distance in mm */
    wait_ms(2000);

    ESP_LOGI(TAG, "backward");
    ESP_ERROR_CHECK(motors_go_dist(-THROTTLE, 50));
    wait_ms(2000);

    /* --- Rotation: left/right for 1 s at 50% throttle --- */
    ESP_LOGI(TAG, "rotate left");
    ESP_ERROR_CHECK(motors_set_throttle2(-THROTTLE, THROTTLE, 1000));
    wait_ms(1500);

    ESP_LOGI(TAG, "rotate right");
    ESP_ERROR_CHECK(motors_set_throttle2(THROTTLE, -THROTTLE, 1000));
    wait_ms(1500);

    /* --- Lift: up and down --- */
    ESP_LOGI(TAG, "lift up");
    lift_set_height_at(LIFT_HEIGHT_HIGH, 50);
    wait_ms(1500);

    ESP_LOGI(TAG, "lift down");
    lift_set_height_at(LIFT_HEIGHT_LOW, 50);
    wait_ms(1500);

    /* --- Head: min -> max angle --- */
    ESP_LOGI(TAG, "head down (%d deg)", HEAD_ANGLE_MIN);
    head_set_angle_at(HEAD_ANGLE_MIN, 50);
    wait_ms(1500);

    ESP_LOGI(TAG, "head up (%d deg)", HEAD_ANGLE_MAX);
    head_set_angle_at(HEAD_ANGLE_MAX, 50);
    wait_ms(1500);

    /* --- Power everything off --- */
    ESP_ERROR_CHECK(subc_stop());

    ESP_LOGI(TAG, "test done");
    vTaskDelete(NULL);
}
