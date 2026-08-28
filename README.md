# Cozmars robot - open cozmo

Cozmars robot controller(ESP32S3R8) firmware.

This is a unfinished project, I only have a few examples to test the components.

## Build and flash

Built with **ESP-IDF v6.0.2**. 

```sh
git clone https://github.com/hyansuper/cozmars.git
git clone https://github.com/hyansuper/cozmars-sub.git
cd cozmars/examples/xxx
idf.py build
idf.py flash monitor
```

## System Diagram

```
┌──────────────┐
│  MAIN        │─── SPI ──────────────────────►┌────────────┐
│  ESP32-S3    │                               │  Screen    │
│              │                               │  ST7789    │
│              │                               └────────────┘
│              │
│              │                            ┌────────────┐
│              │─── DVP ───────────────────►│   Camera   │
│              │                            │   GC0308   │
│              │─── I2C_0 ───┬─────────────►│    120°    │
│              │             │              └────────────┘
│              │             │           
│              │             │    ┌─────────────┐
│              │             └───►│  Audio      │
│              │                  │ ES7210◄─────┼── MIC
│              │─── I2S ─────────►│ ES8311      │───►┌─────┐───►SPK
│              │                  └─────────────┘    │ AMP │
│              │                                     └─────┘
│              │               ┌─────────────►┌──────────┐
│              │               │              │  IMU     │
│              │               │              │  (6-ax)  │
│              │               │              └──────────┘
│              │               │
│              │               ├─────────────►┌──────────┐
│              │               │              │  TOF     │
│              │               │              │  (range) │
│              │               │              └──────────┘
│              │               │
│              │─── I2C_1 ─────┴─────────────►┌──────────────────┐
│              │                              │  SUB             │
│              │                              │  ESP32-C3        │
└──────────────┘                              └─┬──┬──┬──┬──┬──┬─┘
                                                │  │  │  │  │  │
              ┌─────────────────────────────────┘  │  │  │  │  │
              │                                    │  │  │  │  │
              │        ┌───────────────────────────┘  │  │  │  │
              │        │        ┌─────────────────────┘  │  │  │
              │        │        │         ┌──────────────┘  │  │
              │        │        │         │       ┌─────────┘  │
              ▼        ▼        ▼         ▼       ▼            ▼
      ┌───────┐ ┌───────┐ ┌───────┐ ┌───────┐ ┌────────┐ ┌────────┐
      │MOTORS │ │SERVOS │ │ WLED  │ │TOUCH  │ │BATTERY │ │IR      │
      │       │ │       │ │WS2812 │ │       │ │ADC     │ │sensors │
      └───────┘ └───────┘ └───────┘ └───────┘ └────────┘ └────────┘
```

Pin map can be found in [head_config.h](components/cozmars/include/head_config.h).


## Related resource

- https://github.com/hyansuper/cozmars-sub
- https://github.com/hyansuper/cozmars_v3_hardware