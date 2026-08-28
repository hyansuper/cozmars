#pragma once
#include "esp_err.h"
#include "sub_i2c_msg.h"
#include <stdbool.h>


esp_err_t subc_init(void);
esp_err_t subc_reset(void);
esp_err_t subc_sleep(void);
void subc_wakeup(void);
esp_err_t subc_stop(void);
esp_err_t subc_check_version(void);
esp_err_t subc_send_msg(const sub_msg_t *msg);
esp_err_t subc_read_state(sub_state_resp_t *state);
esp_err_t subc_cliff_detection(bool en);
