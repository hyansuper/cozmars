#include <string.h>
#include <stdlib.h>
#include "esp_log.h"
#include "esp_check.h"
#include "driver/i2s_std.h"
#include "driver/i2s_tdm.h"
#include "esp_codec_dev.h"
#include "esp_codec_dev_defaults.h"
#include "head_config.h"
#include "i2c_bus.h"
#include "codec.h"
#include "util.h"

static const char *TAG = "audio_codec";

static esp_codec_dev_handle_t output_dev = NULL;
static esp_codec_dev_handle_t input_dev = NULL;

static bool input_enabled = false;
static bool output_enabled = false;
static int current_volume = AUDIO_OUTPUT_DEFAULT_VOL;

/* Selected mics on the ES7210 and currently captured subset */
static uint32_t input_mics;
static uint32_t capture_mics;
static bool tdm_mode;

/* Board wiring: physical MIC1 -> slot0, MIC3 (echo ref) -> slot1, MIC2 -> slot2 */
typedef struct {
    uint32_t code;
    uint8_t es_mic;      /* ES7210_SEL_* */
    uint8_t tdm_slot;    /* wire slot index in TDM mode */
} chan_map_t;

static const chan_map_t CHAN_MAP[] = {
    { .code = CODEC_MIC1, .es_mic = ES7210_SEL_MIC1, .tdm_slot = 0,},
    { .code = CODEC_MIC2, .es_mic = ES7210_SEL_MIC2, .tdm_slot = 2,},
    { .code = CODEC_REF,  .es_mic = ES7210_SEL_MIC3, .tdm_slot = 1,},
};

esp_err_t codec_init(uint32_t mics)
{
    i2s_chan_handle_t tx_handle = NULL;
    i2s_chan_handle_t rx_handle = NULL;
    const audio_codec_data_if_t *data_if = NULL;
    const audio_codec_ctrl_if_t *out_ctrl_if = NULL;
    const audio_codec_if_t *out_codec_if = NULL;
    const audio_codec_ctrl_if_t *in_ctrl_if = NULL;
    const audio_codec_if_t *in_codec_if = NULL;
    const audio_codec_gpio_if_t *gpio_if = NULL;

    if (mics == 0 || (mics & ~(CODEC_MIC1 | CODEC_MIC2 | CODEC_REF))) {
        return ESP_ERR_INVALID_ARG;
    }
    input_mics = mics;
    if (mics & CODEC_MIC2) mics |= CODEC_MIC1;
    if (mics & CODEC_REF)  mics |= CODEC_MIC1 | CODEC_MIC2;
    capture_mics = 0;
    tdm_mode = __builtin_popcount(mics) >= 3;

    ESP_LOGI(TAG, "Init I2S duplex channels");

    i2s_chan_config_t chan_cfg = {
        .id = I2S_NUM_0,
        .role = I2S_ROLE_MASTER,
        .dma_desc_num = AUDIO_CODEC_DMA_DESC_NUM,
        .dma_frame_num = AUDIO_CODEC_DMA_FRAME_NUM,
        .auto_clear_after_cb = true,
        .auto_clear_before_cb = false,
        .intr_priority = 0,
    };
    ESP_RETURN_ON_ERROR(i2s_new_channel(&chan_cfg, &tx_handle, &rx_handle),
                        TAG, "i2s_new_channel failed");

    i2s_std_config_t std_cfg = {
        .clk_cfg = {
            .sample_rate_hz = AUDIO_SAMPLE_RATE,
            .clk_src = I2S_CLK_SRC_DEFAULT,
            .ext_clk_freq_hz = 0,
            .mclk_multiple = I2S_MCLK_MULTIPLE_256,
        },
        .slot_cfg = {
            .data_bit_width = I2S_DATA_BIT_WIDTH_16BIT,
            .slot_bit_width = I2S_SLOT_BIT_WIDTH_AUTO,
            .slot_mode = I2S_SLOT_MODE_STEREO,
            .slot_mask = I2S_STD_SLOT_BOTH,
            .ws_width = I2S_DATA_BIT_WIDTH_16BIT,
            .ws_pol = false,
            .bit_shift = true,
            .left_align = true,
            .big_endian = false,
            .bit_order_lsb = false,
        },
        .gpio_cfg = {
            .mclk = I2S_MCLK_IO,
            .bclk = I2S_BCLK_IO,
            .ws = I2S_LRCK_IO,
            .dout = I2S_DOUT_IO,
            .din = I2S_GPIO_UNUSED,
            .invert_flags = {0},
        },
    };
    ESP_RETURN_ON_ERROR(i2s_channel_init_std_mode(tx_handle, &std_cfg),
                        TAG, "TX init failed");

    if (tdm_mode) {
        i2s_tdm_config_t tdm_cfg = {
            .clk_cfg = {
                .sample_rate_hz = AUDIO_SAMPLE_RATE,
                .clk_src = I2S_CLK_SRC_DEFAULT,
                .ext_clk_freq_hz = 0,
                .mclk_multiple = I2S_MCLK_MULTIPLE_256,
                .bclk_div = 8,
            },
            .slot_cfg = {
                .data_bit_width = I2S_DATA_BIT_WIDTH_16BIT,
                .slot_bit_width = I2S_SLOT_BIT_WIDTH_AUTO,
                .slot_mode = I2S_SLOT_MODE_STEREO,
                .slot_mask = I2S_TDM_SLOT0 | I2S_TDM_SLOT1 | I2S_TDM_SLOT2,
                .ws_width = I2S_TDM_AUTO_WS_WIDTH,
                .ws_pol = false,
                .bit_shift = true,
                .left_align = false,
                .big_endian = false,
                .bit_order_lsb = false,
                .skip_mask = false,
                .total_slot = 4,
            },
            .gpio_cfg = {
                .mclk = I2S_MCLK_IO,
                .bclk = I2S_BCLK_IO,
                .ws = I2S_LRCK_IO,
                .dout = I2S_GPIO_UNUSED,
                .din = I2S_DIN_IO,
                .invert_flags = {0},
            },
        };
        ESP_RETURN_ON_ERROR(i2s_channel_init_tdm_mode(rx_handle, &tdm_cfg),
                            TAG, "RX init failed");
    } else {
        i2s_std_config_t rx_std_cfg = std_cfg;
        rx_std_cfg.gpio_cfg.dout = I2S_GPIO_UNUSED;
        rx_std_cfg.gpio_cfg.din = I2S_DIN_IO;
        ESP_RETURN_ON_ERROR(i2s_channel_init_std_mode(rx_handle, &rx_std_cfg),
                            TAG, "RX init failed");
    }

    ESP_RETURN_ON_ERROR(i2s_channel_enable(tx_handle), TAG, "TX enable failed");
    ESP_RETURN_ON_ERROR(i2s_channel_enable(rx_handle), TAG, "RX enable failed");

    ESP_LOGI(TAG, "Create data interface");
    audio_codec_i2s_cfg_t i2s_cfg = {
        .port = I2S_NUM_0,
        .rx_handle = rx_handle,
        .tx_handle = tx_handle,
    };
    data_if = audio_codec_new_i2s_data(&i2s_cfg);
    if (!data_if) {
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Create control interface for ES8311 (addr=0x%02x)", ES8311_I2C_ADDR);
    audio_codec_i2c_cfg_t i2c_cfg = {
        .port = I2C_NUM_0,
        .addr = ES8311_I2C_ADDR,
        .bus_handle = i2c_onboard_handle,
    };
    out_ctrl_if = audio_codec_new_i2c_ctrl(&i2c_cfg);
    if (!out_ctrl_if) {
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Create control interface for ES7210 (addr=0x%02x)", ES7210_I2C_ADDR);
    i2c_cfg.addr = ES7210_I2C_ADDR;
    in_ctrl_if = audio_codec_new_i2c_ctrl(&i2c_cfg);
    if (!in_ctrl_if) {
        return ESP_FAIL;
    }

    gpio_if = audio_codec_new_gpio();
    if (!gpio_if) {
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Init ES8311 codec (DAC mode, pa_pin=%d)", AMP_ENABLE_IO);
    es8311_codec_cfg_t es8311_cfg = {
        .ctrl_if = out_ctrl_if,
        .gpio_if = gpio_if,
        .codec_mode = ESP_CODEC_DEV_WORK_MODE_DAC,
        .master_mode = false,
        .pa_pin = AMP_ENABLE_IO,
        .pa_reverted = false,
        .use_mclk = true,
        .hw_gain = {
            .pa_voltage = 5.0,
            .codec_dac_voltage = 3.3,
        },
    };
    out_codec_if = es8311_codec_new(&es8311_cfg);
    if (!out_codec_if) {
        return ESP_FAIL;
    }

    es7210_codec_cfg_t es7210_cfg = {
        .ctrl_if = in_ctrl_if,
        .master_mode = false,
        .mic_selected = 0,
        // .mclk_src = ES7210_MCLK_FROM_PAD,
        // .mclk_div = 256,
    };
    for (int i = 0; i < ARRAY_SIZE(CHAN_MAP); i++) {
        if (mics & CHAN_MAP[i].code) {
            es7210_cfg.mic_selected |= CHAN_MAP[i].es_mic;
        }
    }
    ESP_LOGI(TAG, "Init ES7210 codec (mics=0x%x, powered=0x%x%s)", input_mics, mics,
             tdm_mode ? ", TDM slots [mic1 ref mic2]" : ", stereo");
    in_codec_if = es7210_codec_new(&es7210_cfg);
    if (!in_codec_if) {
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Create output device handle");
    esp_codec_dev_cfg_t dev_cfg = {
        .dev_type = ESP_CODEC_DEV_TYPE_OUT,
        .codec_if = out_codec_if,
        .data_if = data_if,
    };
    output_dev = esp_codec_dev_new(&dev_cfg);
    if (!output_dev) {
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Create input device handle");
    dev_cfg.dev_type = ESP_CODEC_DEV_TYPE_IN;
    dev_cfg.codec_if = in_codec_if;
    input_dev = esp_codec_dev_new(&dev_cfg);
    if (!input_dev) {
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Audio codec initialized (rate=%d)", AUDIO_SAMPLE_RATE);
    return ESP_OK;
}

int codec_read(int16_t *dest, int samples)
{
    if (input_enabled) {
        ESP_ERROR_CHECK_WITHOUT_ABORT(esp_codec_dev_read(input_dev, (void*)dest, samples * sizeof(int16_t)));
    }
    return samples;
}

int codec_write(const int16_t *data, int samples)
{
    if (output_enabled) {
        ESP_ERROR_CHECK_WITHOUT_ABORT(esp_codec_dev_write(output_dev, (void*)data, samples * sizeof(int16_t)));
    }
    return samples;
}

esp_err_t codec_set_volume(int volume)
{
    volume = clamp_int(volume, 0, 100);
    if (output_enabled && output_dev) {
        ESP_RETURN_ON_ERROR(esp_codec_dev_set_out_vol(output_dev, volume),
                            TAG, "set_out_vol failed");
    }
    current_volume = volume;
    return ESP_OK;
}

int codec_get_volume(void)
{
    return current_volume;
}

esp_err_t codec_output_set_enable(bool enable)
{
    if (enable == output_enabled) {
        return ESP_OK;
    }
    if (enable) {
        esp_codec_dev_sample_info_t fs = {
            .bits_per_sample = AUDIO_BITS_PER_SAMPLE,
            .channel = 1,
            .channel_mask = 0,
            .sample_rate = AUDIO_SAMPLE_RATE,
            .mclk_multiple = 0,
        };
        ESP_RETURN_ON_ERROR(esp_codec_dev_open(output_dev, &fs),
                            TAG, "output open failed");
        ESP_RETURN_ON_ERROR(esp_codec_dev_set_out_vol(output_dev, current_volume),
                            TAG, "set volume failed");
        ESP_LOGI(TAG, "Output enabled (PA pin high)");
    } else {
        ESP_RETURN_ON_ERROR(esp_codec_dev_close(output_dev),
                            TAG, "output close failed");
        ESP_LOGI(TAG, "Output disabled (PA pin low)");
    }
    output_enabled = enable;
    return ESP_OK;
}

bool codec_output_is_enabled(void)
{
    return output_enabled;
}

static esp_err_t input_enable_impl(bool enable, uint32_t mics)
{
    if (!enable) {
        if (!input_enabled) {
            return ESP_OK;
        }
        ESP_RETURN_ON_ERROR(esp_codec_dev_close(input_dev),
                            TAG, "input close failed");
        input_enabled = false;
        capture_mics = 0;
        ESP_LOGI(TAG, "Input disabled");
        return ESP_OK;
    }

    if (mics == 0 || (mics & ~input_mics)) {
        return ESP_ERR_INVALID_ARG;
    }
    if (input_enabled) {
        return (capture_mics == mics) ? ESP_OK : ESP_ERR_INVALID_STATE;
    }

    esp_codec_dev_sample_info_t fs = {
        .bits_per_sample = AUDIO_BITS_PER_SAMPLE,
        .sample_rate = AUDIO_SAMPLE_RATE,
        .mclk_multiple = 0,
    };
    if (tdm_mode) {
        fs.channel = 4; /* keep total_slot = 4 */
        for (int i = 0; i < ARRAY_SIZE(CHAN_MAP); i++) {
            if (mics & CHAN_MAP[i].code) {
                fs.channel_mask |= ESP_CODEC_DEV_MAKE_CHANNEL_MASK(CHAN_MAP[i].tdm_slot);
            }
        }
    } else {
        /* Never use channel==1: esp_codec_dev rewrites such opens to
         * left-channel capture, so MIC2-only would silently get MIC1. */
        fs.channel = 2;
        if (mics & CODEC_MIC1) fs.channel_mask |= ESP_CODEC_DEV_MAKE_CHANNEL_MASK(0);
        if (mics & CODEC_MIC2) fs.channel_mask |= ESP_CODEC_DEV_MAKE_CHANNEL_MASK(1);
    }
    capture_mics = mics;

    ESP_RETURN_ON_ERROR(esp_codec_dev_open(input_dev, &fs),
                        TAG, "input open failed");

    /* Gain masks use physical mic numbering: CH0=MIC1, CH1=MIC2, CH2=MIC3(ref) */
    int gain_mask = ((mics & CODEC_MIC1) ? ESP_CODEC_DEV_MAKE_CHANNEL_MASK(0): 0) | ((mics & CODEC_MIC2) ? ESP_CODEC_DEV_MAKE_CHANNEL_MASK(1): 0);
    if (gain_mask) {
        ESP_ERROR_CHECK(esp_codec_dev_set_in_channel_gain(input_dev, gain_mask, AUDIO_INPUT_GAIN_DB));
    }
#ifdef AUDIO_INPUT_REF_GAIN_DB
    if (mics & CODEC_REF) {
        ESP_ERROR_CHECK(esp_codec_dev_set_in_channel_gain(input_dev, ESP_CODEC_DEV_MAKE_CHANNEL_MASK(2), AUDIO_INPUT_REF_GAIN_DB));
    }
#endif

    input_enabled = true;
    ESP_LOGI(TAG, "Input enabled (channels=0x%x)", mics);
    return ESP_OK;
}

esp_err_t codec_input_set_enable(bool enable)
{
    return input_enable_impl(enable, input_mics);
}

esp_err_t codec_input_enable_mic(uint32_t mics)
{
    return input_enable_impl(true, mics);
}

bool codec_input_is_enabled(void)
{
    return input_enabled;
}

int codec_input_channels(void)
{
    return __builtin_popcount(capture_mics);
}
