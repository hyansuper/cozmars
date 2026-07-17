/* Record 5 seconds of audio from the mic and play it back on the speaker.
 *
 * Single-mic mono capture: the ES7210 runs in stereo mode and only the
 * left slot (MIC1) is captured.
 */
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "i2c_bus.h"
#include "codec.h"
#include "head_config.h"

#define RECORD_SECONDS 5

static const char *TAG = "record_and_playback";

void app_main(void)
{
    const int total_samples = AUDIO_SAMPLE_RATE * RECORD_SECONDS;

    int16_t *buf = heap_caps_malloc(total_samples * sizeof(int16_t),
                                    MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!buf) {
        ESP_LOGE(TAG, "failed to allocate record buffer");
        return;
    }

    ESP_ERROR_CHECK(i2c_buses_init());
    ESP_ERROR_CHECK(codec_init(CODEC_MIC1));

    while (1) {
        /* --- record --- */
        ESP_LOGI(TAG, "recording %d seconds...", RECORD_SECONDS);
        ESP_ERROR_CHECK(codec_input_set_enable(true));

        int recorded = 0;
        while (recorded < total_samples) {
            int n = codec_read(buf + recorded, total_samples - recorded);
            if (n <= 0) {
                ESP_LOGE(TAG, "codec_read failed");
                goto out;
            }
            recorded += n;
        }
        codec_input_set_enable(false);
        ESP_LOGI(TAG, "recorded %d samples", recorded);

        /* --- playback --- */
        ESP_LOGI(TAG, "playing back...");
        ESP_ERROR_CHECK(codec_output_set_enable(true));

        for (int pos = 0; pos < recorded;) {
            int n = codec_write(buf + pos, recorded - pos);
            if (n <= 0) {
                ESP_LOGE(TAG, "codec_write failed");
                break;
            }
            pos += n;
        }
        codec_output_set_enable(false);
    }

out:
    codec_output_set_enable(false);
    free(buf);
}
