#pragma once
#include "esp_err.h"
#include "esp_wifi.h"

esp_err_t wifi_init(void);
esp_err_t wifi_get_rssi(int8_t *rssi); // only useful in sta mode
esp_err_t wifi_get_ip(char *ip_str, size_t len); // only useful in sta mode

// WIFI_PS_MAX_MODEM / WIFI_PS_MIN_MODEM / WIFI_PS_NONE
static inline esp_err_t wifi_set_power_save_mode(wifi_ps_type_t ps) {
	return esp_wifi_set_ps(ps);
}