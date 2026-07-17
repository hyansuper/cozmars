#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

/* Input channel selectors, OR-able */
#define CODEC_MIC1 (1u << 0)
#define CODEC_MIC2 (1u << 1)
#define CODEC_REF  (1u << 2)

/* Select which mics the ES7210 powers up. 3+ channels put the chip in
 * 4-slot TDM mode, fewer use plain stereo. */
esp_err_t codec_init(uint32_t mics);

/* Reads interleaved samples of all captured channels;
 * use codec_input_channels() to know the frame stride. */
int codec_read(int16_t *dest, int samples);
int codec_write(const int16_t *data, int samples);

esp_err_t codec_set_volume(int volume);
int codec_get_volume(void);

/* Enable/disable input, capturing all mics selected in codec_init */
esp_err_t codec_input_set_enable(bool enable);
/* enable input with Selected captured channels (must be a subset of codec_init's mask).*/
esp_err_t codec_input_enable_mic(uint32_t mics);
bool codec_input_is_enabled(void);
/* Number of interleaved channels in the current capture stream */
int codec_input_channels(void);

esp_err_t codec_output_set_enable(bool enable);
bool codec_output_is_enabled(void);
