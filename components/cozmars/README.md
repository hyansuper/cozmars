# cozmars

Core component for the cozmars-head robot: display (ST7789 via LovyanGFX), I2C buses, audio codec (ES8311/ES7210), sub-processor control (motors, servos, WLED), and camera (GC0308 over DVP).

## Recommended configuration

If you are building an app that uses this component to control the cozmars robot, use the [recommended sdkconfig](sdkconfig.defaults.recommend).

It sets the board-specific options (16MB flash, octal PSRAM, GC0308 sensor formats, codec chip support).

## Hardware initialization

See [example.c](example.c) for the proper init order:

Note that `i2c_buses_init()` must run before `codec_init()`, `subc_init()`, and `cam_init()`.
