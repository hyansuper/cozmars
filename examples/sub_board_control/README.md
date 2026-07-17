# Sub Board Control Example

Tests all sub-board (ESP32-C3 co-processor) peripherals while a background task prints sensor readings once per second.

Test sequence:

1. **WLED** — blink (red) and fade (green), 3 repeats each
2. **Motors** — forward and backward 5 cm, then rotate left and right for 1 s (all at 50 % throttle)
3. **Lift** — up and down
4. **Head** — min to max angle (-20 to 45 degrees)

At the end everything is powered off with a single emergency-stop command (`subc_stop`).

The background task reports: battery level/charging status, cliff-detection IR sensors, back touch sensor, and idle flags (head / lift / WLED / motors).

## WARNING

You must properly configure the servos on the sub board before running this test — otherwise the servos can be driven past their mechanical limits and damaged.

## Build & Flash

```sh
idf.py -C examples/sub_board_control build
idf.py -C examples/sub_board_control -p /dev/ttyUSB0 flash monitor
```

Uses the shared components in `components/` via `EXTRA_COMPONENT_DIRS` and `components/cozmars/sdkconfig.defaults.recommend` as sdkconfig defaults.
