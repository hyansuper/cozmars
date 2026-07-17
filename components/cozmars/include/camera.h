#pragma once
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

typedef enum {
    CAM_STATE_CLOSED = 0,   /*!< cam_deinit() has been called, hardware is powered down */
    CAM_STATE_INITIALIZED,  /*!< cam_init() has been called, no frame capture in progress */
    CAM_STATE_STREAMING,    /*!< cam_start() has been called, frames can be captured */
} cam_state_t;

typedef struct {
    uint32_t width;    /*!< Frame width in pixels */
    uint32_t height;   /*!< Frame height in pixels */
    uint32_t fps;      /*!< Native sensor framerate for this resolution */
} cam_format_info_t;

/* Initialize the camera (DVP + GC0308) and open the V4L2 device. Must be
 * called after i2c_buses_init(). */
esp_err_t cam_init(void);

/* Close the V4L2 device, deinitialize the video subsystem and power the sensor down. */
void cam_deinit(void);

/* Put the GC0308 sensor into power-down mode by asserting CAM_PWDN_IO.
 * Power is restored automatically on the next cam_init(). */
esp_err_t cam_power_down(void);

/* Wake the GC0308 sensor from power-down by releasing CAM_PWDN_IO (active-low).
 * The sensor resumes with its retained register configuration. */
esp_err_t cam_power_up(void);

/* Return the current camera state. */
cam_state_t cam_get_state(void);

/* Begin frame capture. Allocates mmap'd buffers and starts the DVP stream. */
esp_err_t cam_start(void);

/* Stop frame capture and release the allocated buffers. */
esp_err_t cam_stop(void);

/* Capture the next frame. On success *buf points to the frame data and *size
 * holds its size in bytes. The buffer is owned by the camera and stays valid
 * until the next cam_capture() call or cam_stop(). */
esp_err_t cam_capture(uint8_t **buf, size_t *size);

