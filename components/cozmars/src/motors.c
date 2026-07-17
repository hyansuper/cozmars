#include "motors.h"

esp_err_t motors_go(int th)
{
    return motors_set_throttle2(th, th, 0);
}

esp_err_t motors_go2(int th1, int th2)
{
    return motors_set_throttle2(th1, th2, 0);
}

esp_err_t motors_set_throttle(int th, uint32_t dur)
{
    return motors_set_throttle2(th, th, dur);
}

esp_err_t motors_set_throttle2(int th1, int th2, uint32_t dur)
{
    return subc_send_msg(&(sub_msg_t){
        .type = SUB_MSG_WR_MOTOR,
        .motor_cmd.type = MOTOR_CMD_SET_THROTTLE,
        .motor_cmd.set_throttle = {
            .throttle = {th1, th2},
            .duration = dur,
        },
    });
}

esp_err_t motors_go_dist(int th, int dist)
{
    return subc_send_msg(&(sub_msg_t){
        .type = SUB_MSG_WR_MOTOR,
        .motor_cmd.type = MOTOR_CMD_GO_DISTANCE,
        .motor_cmd.go_distance = {
            .distance = dist,
            .throttle = th,
        },
    });
}

esp_err_t motors_off(void)
{
    return subc_send_msg(&(sub_msg_t){
        .type = SUB_MSG_WR_MOTOR,
        .motor_cmd.type = MOTOR_CMD_POWEROFF,
    });
}
