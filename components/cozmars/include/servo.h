#include "sub_ctl.h"

void head_set_default_speed(int speed);
int head_get_default_speed();
esp_err_t head_set_angle(int ang); // set angle at default speed
esp_err_t head_set_angle_at(int ang, int speed);
esp_err_t head_set_angle_in(int ang, uint32_t dur);
esp_err_t head_off(void);

void lift_set_default_speed(int speed);
int lift_get_default_speed();
void lift_set_height(int height); // set height % at default speed
esp_err_t lift_set_height_at(int ang, int speed);
esp_err_t lift_set_height_in(int ang, uint32_t dur);
esp_err_t lift_off(void);