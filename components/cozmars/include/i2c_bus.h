#pragma once
#include "esp_err.h"
#include "driver/i2c_master.h"

extern i2c_master_bus_handle_t i2c_onboard_handle;
extern i2c_master_bus_handle_t i2c_ext_handle;

esp_err_t i2c_buses_init(void);

/* Scan all 7-bit addresses on the given bus, log and return the number of devices found */
int i2c_bus_scan(i2c_master_bus_handle_t bus);
