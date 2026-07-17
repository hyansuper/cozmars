# Display Eye Example

Initializes the ST7789 display and draws two cyan rectangular eyes (Cozmo robot style) on a black background. The eyes blink from time to time — they shortly collapse to a line and reopen.

## Build & Flash

```sh
idf.py -C examples/display_eye build
idf.py -C examples/display_eye -p /dev/ttyUSB0 flash monitor
```

Uses the shared components in `components/` via `EXTRA_COMPONENT_DIRS` and `components/cozmars/sdkconfig.defaults.recommend` as sdkconfig defaults.
