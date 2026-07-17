#include <string.h>
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "head_config.h"
#include "i2c_bus.h"
#include "sub_i2c_msg.h"
#include "sub_ctl.h"

static const char *TAG = "sub";

static i2c_master_dev_handle_t sub_dev;

esp_err_t subc_send_msg(const sub_msg_t *msg)
{
    return i2c_master_transmit(sub_dev, (const uint8_t *)msg, sizeof(*msg), pdMS_TO_TICKS(100));
}

esp_err_t subc_read_state(sub_state_resp_t *state)
{
    sub_msg_type_t type = SUB_MSG_RD_STATE;
    return i2c_master_transmit_receive(sub_dev, (const uint8_t *)&type, sizeof(type),
                                       (uint8_t *)state, sizeof(*state), pdMS_TO_TICKS(100));
}

esp_err_t subc_check_version(void)
{
    sub_msg_version_resp_t ver;
    sub_msg_type_t type = SUB_MSG_RD_VERSION;
    esp_err_t ret = i2c_master_transmit_receive(sub_dev, (const uint8_t *)&type, sizeof(type),
                                                (uint8_t *)&ver, sizeof(ver), pdMS_TO_TICKS(100));
    if (ret != ESP_OK) {
        return ret;
    }

    sub_msg_version_resp_t expected = GET_SUB_MSG_VER_RESP();
    if (memcmp(&ver, &expected, sizeof(expected)) != 0) {
        ESP_LOGE(TAG, "version mismatch: got %d.%d.%d, expected %d.%d.%d",
                 ver.major, ver.minor, ver.patch,
                 expected.major, expected.minor, expected.patch);
        return ESP_ERR_INVALID_VERSION;
    }
    ESP_LOGI(TAG, "sub version: %d.%d.%d", ver.major, ver.minor, ver.patch);
    return ESP_OK;
}

esp_err_t subc_sleep(void)
{
    return subc_send_msg(&(sub_msg_t){.type=SUB_MSG_WR_POWER, .power_cmd.type=POWER_CMD_SLEEP});
}

/* put servos and motors to powerless mode (pwm duty=0), can be useful in emergency */
esp_err_t subc_stop(void)
{
    return subc_send_msg(&(sub_msg_t){.type=SUB_MSG_WR_STOP});
}

/* 
    sub chip will be waken up if scl pin low. we can simply send some garbage to sub.
    but we should wait a few seconds before the sub can process true commands.
*/
void subc_wakeup(void)
{
    uint8_t buf = 0;
    i2c_master_transmit(sub_dev, &buf, sizeof(buf), pdMS_TO_TICKS(100));
}

esp_err_t subc_reset(void)
{
    return subc_send_msg(&(sub_msg_t){.type=SUB_MSG_WR_POWER, .power_cmd.type=POWER_CMD_REBOOT});
}

esp_err_t subc_init(void)
{
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = SUB_I2C_ADDR,
        .scl_speed_hz = 400000,
        .scl_wait_us = 0,
        .flags = {
            .disable_ack_check = false,
        },
    };
    return i2c_master_bus_add_device(i2c_ext_handle, &dev_cfg, &sub_dev);
}
