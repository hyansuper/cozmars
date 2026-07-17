#include "servo.h"

#define SERVO_DEFAULT_HOLD_MS 60

static int head_default_speed = 50;
static int lift_default_speed = 50;

static esp_err_t servo_set_value(sub_msg_type_t target, servo_cmd_type_t type, int value, uint32_t speed_or_dur)
{
    return subc_send_msg(&(sub_msg_t){
        .type = target,
        .servo_cmd.type = type,
        .servo_cmd.set_value = {
            .value = value,
            .hold = SERVO_DEFAULT_HOLD_MS,
            .speed = speed_or_dur,
        },
    });
}

static esp_err_t servo_poweroff(sub_msg_type_t target)
{
    return subc_send_msg(&(sub_msg_t){
        .type = target,
        .servo_cmd.type = SERVO_CMD_POWEROFF,
    });
}

esp_err_t head_off(void)
{
    return servo_poweroff(SUB_MSG_WR_HEAD);
}

void head_set_default_speed(int speed)
{
    head_default_speed = speed;
}

int head_get_default_speed(void)
{
    return head_default_speed;
}

esp_err_t head_set_angle_at(int ang, int speed)
{
    return servo_set_value(SUB_MSG_WR_HEAD, SERVO_CMD_SET_VALUE_AT_SPEED, ang, speed);
}

esp_err_t head_set_angle(int ang)
{
    return head_set_angle_at(ang, head_default_speed);
}

esp_err_t head_set_angle_in(int ang, uint32_t dur)
{
    return servo_set_value(SUB_MSG_WR_HEAD, SERVO_CMD_SET_VALUE_IN_DURATION, ang, dur);
}

void lift_set_default_speed(int speed)
{
    lift_default_speed = speed;
}

int lift_get_default_speed(void)
{
    return lift_default_speed;
}

esp_err_t lift_set_height_at(int height, int speed)
{
    return servo_set_value(SUB_MSG_WR_LIFT, SERVO_CMD_SET_VALUE_AT_SPEED, height, speed);
}

esp_err_t lift_set_height_in(int height, uint32_t dur)
{
    return servo_set_value(SUB_MSG_WR_LIFT, SERVO_CMD_SET_VALUE_IN_DURATION, height, dur);
}

esp_err_t lift_off(void)
{
    return servo_poweroff(SUB_MSG_WR_LIFT);
}

void lift_set_height(int height)
{
    lift_set_height_at(height, lift_default_speed);
}
