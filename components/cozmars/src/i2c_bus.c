#include "esp_log.h"
#include "head_config.h"
#include "i2c_bus.h"

static const char *TAG = "i2c_bus";

i2c_master_bus_handle_t i2c_onboard_handle = NULL;
i2c_master_bus_handle_t i2c_ext_handle = NULL;

static esp_err_t i2c_bus_init(i2c_master_bus_handle_t* handle, i2c_port_t port, gpio_num_t scl, gpio_num_t sda)
{
    ESP_LOGI(TAG, "Init I2C bus %d (SDA=%d, SCL=%d)", port, sda, scl);
    i2c_master_bus_config_t cfg = {
        .i2c_port = port,
        .sda_io_num = sda,
        .scl_io_num = scl,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = false, // hardware pullup
    };
    return i2c_new_master_bus(&cfg, handle);
}

esp_err_t i2c_buses_init(void)
{
    return i2c_bus_init(&i2c_onboard_handle, I2C_NUM_0, I2C_ONBOARD_SCL_IO, I2C_ONBOARD_SDA_IO) 
            || i2c_bus_init(&i2c_ext_handle, I2C_NUM_1, I2C_EXT_SCL_IO, I2C_EXT_SDA_IO);
}

int i2c_bus_scan(i2c_master_bus_handle_t bus)
{
    int found = 0;
    for (uint16_t addr = 0x01; addr <= 0x7F; addr++) {
        if (addr >= 0x78 && addr <= 0x7F) {
            continue; // reserved for 10-bit addressing
        }
        esp_err_t ret = i2c_master_probe(bus, addr, 50);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Found device at 0x%02X", addr);
            found++;
        } else if (ret != ESP_ERR_NOT_FOUND) {
            ESP_LOGW(TAG, "Probe 0x%02X failed: %s", addr, esp_err_to_name(ret));
        }
    }
    ESP_LOGI(TAG, "Scan done, %d device(s) found", found);
    return found;
}
