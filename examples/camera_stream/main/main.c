#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "i2c_bus.h"
#include "wifi.h"
#include "camera.h"
#include "camera_server.h"

static const char *TAG = "camera_stream";

void app_main(void)
{
    ESP_ERROR_CHECK(wifi_init());
    ESP_ERROR_CHECK(i2c_buses_init());
    ESP_ERROR_CHECK(cam_init());

    ESP_ERROR_CHECK(camera_server_start());

    ESP_LOGI(TAG, "camera stream ready");
    vTaskDelete(NULL);
}
