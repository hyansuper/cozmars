#include "sub_ctl.h"

esp_err_t motors_go(int th); // set both throttle to th, non-stop
esp_err_t motors_go2(int th1, int th2);

esp_err_t motors_set_throttle(int th, uint32_t dur);
esp_err_t motors_set_throttle2(int th1, int th2, uint32_t dur);

esp_err_t motors_go_dist(int th, int dist);

esp_err_t motors_off();