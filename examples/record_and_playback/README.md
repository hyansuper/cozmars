# Record and Play Example

Records 5 seconds from MIC1 (mono, 16 kHz / 16-bit) into PSRAM, then plays it back on the mono speaker (ES8311 + PA).

Audio init lives in `components/cozmars` (`codec.c`), built on [esp_codec_dev](https://components.espressif.com/components/espressif/esp_codec_dev) — the codec device stack used by ESP-ADF audio boards. The example selects `CODEC_MIC1` only, so the ES7210 runs in stereo mode and just the left slot (MIC1) is captured.

## Build & Flash

```sh
idf.py -C examples/record_and_playback build
idf.py -C examples/record_and_playback -p /dev/ttyUSB0 flash monitor
```

Uses the shared components in `components/` via `EXTRA_COMPONENT_DIRS` and `components/cozmars/sdkconfig.defaults.recommend` as sdkconfig defaults.

## Notes

- The example selects `CODEC_MIC1` only, so the ES7210 runs in stereo mode and just the left slot (MIC1) is captured. Selecting 3+ channels (or any channel with `CODEC_REF`) switches the chip to 4-slot TDM automatically; extra physical mics are powered as needed.
- Input gain comes from `AUDIO_INPUT_GAIN_DB` in `head_config.h`; output volume from `AUDIO_OUTPUT_DEFAULT_VOL`.
