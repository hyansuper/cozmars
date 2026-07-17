# Camera Stream Example

Streams the OV3660/GC0308 camera view over HTTP as an MJPEG stream.

The app initializes Wi-Fi (provisioning portal if no credentials are stored), the I2C buses, and the camera, then starts a web server on port 80:

- `/` — web page displaying the live stream
- `/stream` — raw MJPEG stream (`multipart/x-mixed-replace`)

Frames are captured at 320x240 and JPEG-encoded with `esp_new_jpeg`.

## Build & Flash

```sh
idf.py -C examples/camera_stream build
idf.py -C examples/camera_stream -p /dev/ttyUSB0 flash monitor
```

Open the device's IP address (printed on the serial monitor) in a browser to view the stream.
