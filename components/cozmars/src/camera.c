#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include "esp_log.h"
#include "esp_check.h"
#include "driver/gpio.h"
#include "head_config.h"
#include "i2c_bus.h"
#include "camera.h"
#include "esp_video_init.h"
#include "esp_video_ioctl.h"
#include "esp_video_device.h"
#include "esp_cam_sensor.h"

#define CAM_BUFFER_COUNT 2
#define CAM_SCCB_FREQ_HZ 100000
#define CAM_XCLK_FREQ_HZ 20000000 // 20MHZ is wanted by GC0308

static const char *TAG = "camera";

static cam_state_t s_state = CAM_STATE_CLOSED;
static int s_fd = -1;
static uint8_t *s_buffers[CAM_BUFFER_COUNT];
static size_t s_buffer_lengths[CAM_BUFFER_COUNT];
static int s_pending_index = -1; /* DQBUF'd buffer that has not been requeued yet */

static uint32_t sensor_format_to_v4l2(esp_cam_sensor_output_format_t format)
{
    switch (format) {
    case ESP_CAM_SENSOR_PIXFORMAT_YUV422_YUYV:
        return V4L2_PIX_FMT_YUYV;
    case ESP_CAM_SENSOR_PIXFORMAT_YUV422_UYVY:
        return V4L2_PIX_FMT_UYVY;
    case ESP_CAM_SENSOR_PIXFORMAT_RGB565_LE:
        return V4L2_PIX_FMT_RGB565;
    case ESP_CAM_SENSOR_PIXFORMAT_RGB565_BE:
        return V4L2_PIX_FMT_RGB565X;
    case ESP_CAM_SENSOR_PIXFORMAT_GRAYSCALE:
        return V4L2_PIX_FMT_GREY;
    default:
        return 0;
    }
}

esp_err_t cam_init(void)
{
    const esp_video_init_dvp_config_t dvp_config = {
        .sccb_config = {
            .init_sccb  = false,
            .i2c_handle = i2c_onboard_handle,
            .freq       = CAM_SCCB_FREQ_HZ,
        },
        .reset_pin = CAM_RESET_IO,
        .pwdn_pin  = CAM_PWDN_IO,
        .dvp_pin = {
            .data_width = CAM_CTLR_DATA_WIDTH_8,
            .data_io = {
                CAM_D0_IO, CAM_D1_IO, CAM_D2_IO, CAM_D3_IO,
                CAM_D4_IO, CAM_D5_IO, CAM_D6_IO, CAM_D7_IO,
            },
            .vsync_io = CAM_VSYNC_IO,
            .de_io    = CAM_HREF_IO,
            .pclk_io  = CAM_PCLK_IO,
            .xclk_io  = CAM_XCLK_IO,
        },
        .xclk_freq = CAM_XCLK_FREQ_HZ,
    };

    const esp_video_init_config_t video_config = {
        .dvp = &dvp_config,
    };

    esp_err_t ret;
    ESP_RETURN_ON_ERROR(esp_video_init_with_flags(&video_config, ESP_VIDEO_INIT_FLAGS_DVP),
                        TAG, "failed to initialize video");

    s_fd = open(ESP_VIDEO_DVP_DEVICE_NAME, O_RDONLY);
    ESP_GOTO_ON_FALSE(s_fd >= 0, ESP_FAIL, err, TAG, "failed to open %s", ESP_VIDEO_DVP_DEVICE_NAME);

    // esp_cam_sensor_format_t fmt = { 0 };
    // ESP_GOTO_ON_ERROR(ioctl(s_fd, VIDIOC_G_SENSOR_FMT, &fmt), err, TAG, "failed to get sensor format");
    // s_sensor_format = fmt;

    // s_v4l2_pix_fmt = sensor_format_to_v4l2(fmt.format);
    // ESP_GOTO_ON_FALSE(s_v4l2_pix_fmt, err, TAG, "unsupported sensor pixel format");

    // s_width = fmt.width;
    // s_height = fmt.height;
    // s_fps = fmt.fps;
    // s_skip_frames = 1;
    s_pending_index = -1;

    s_state = CAM_STATE_INITIALIZED;
    ESP_LOGI(TAG, "camera initialized");

    return ESP_OK;

err:
    if (s_fd >= 0) {
        close(s_fd);
        s_fd = -1;
    }
    esp_video_deinit_with_flags(ESP_VIDEO_INIT_FLAGS_DVP);
    return ESP_FAIL;
}

void cam_deinit(void)
{
    if (s_state == CAM_STATE_STREAMING) {
        cam_stop();
    }

    if (s_fd >= 0) {
        close(s_fd);
        s_fd = -1;
    }

    s_state = CAM_STATE_CLOSED;

    esp_err_t ret = esp_video_deinit_with_flags(ESP_VIDEO_INIT_FLAGS_DVP);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "failed to deinitialize video: %s", esp_err_to_name(ret));
        return;
    }

    ESP_LOGI(TAG, "camera deinitialized");

    cam_power_down();
}

esp_err_t cam_power_down(void)
{
    gpio_config_t conf = {
        .pin_bit_mask = 1ULL << CAM_PWDN_IO,
        .mode = GPIO_MODE_OUTPUT,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&conf), TAG, "failed to configure pwdn pin");
    ESP_RETURN_ON_ERROR(gpio_set_level(CAM_PWDN_IO, 1), TAG, "failed to set pwdn pin");

    ESP_LOGI(TAG, "camera powered down");
    return ESP_OK;
}

esp_err_t cam_power_up(void)
{
    ESP_RETURN_ON_ERROR(gpio_set_level(CAM_PWDN_IO, 0), TAG, "failed to set pwdn pin");

    ESP_LOGI(TAG, "camera powered up");
    return ESP_OK;
}

cam_state_t cam_get_state(void)
{
    return s_state;
}

esp_err_t cam_start(void)
{
    const int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    ESP_RETURN_ON_FALSE(s_state == CAM_STATE_INITIALIZED, ESP_ERR_INVALID_STATE,
                        TAG, "camera is not initialized");

    // struct v4l2_format format = {
    //     .type = type,
    //     .fmt.pix.width = s_width,
    //     .fmt.pix.height = s_height,
    //     .fmt.pix.pixelformat = s_v4l2_pix_fmt,
    // };
    // if (ioctl(s_fd, VIDIOC_S_FMT, &format) != 0) {
    //     ESP_LOGE(TAG, "failed to set format");
    //     return ESP_FAIL;
    // }

    struct v4l2_requestbuffers req = {
        .count  = CAM_BUFFER_COUNT,
        .type   = type,
        .memory = V4L2_MEMORY_MMAP,
    };
    if (ioctl(s_fd, VIDIOC_REQBUFS, &req) != 0) {
        ESP_LOGE(TAG, "failed to request buffers");
        return ESP_FAIL;
    }

    for (int i = 0; i < CAM_BUFFER_COUNT; i++) {
        struct v4l2_buffer buf = { 0 };
        buf.type   = type;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index  = i;
        if (ioctl(s_fd, VIDIOC_QUERYBUF, &buf) != 0) {
            ESP_LOGE(TAG, "failed to query buffer %d", i);
            return ESP_FAIL;
        }

        s_buffers[i] = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, s_fd, buf.m.offset);
        if (s_buffers[i] == MAP_FAILED) {
            ESP_LOGE(TAG, "failed to map buffer %d", i);
            s_buffers[i] = NULL;
            return ESP_FAIL;
        }
        s_buffer_lengths[i] = buf.length;

        if (ioctl(s_fd, VIDIOC_QBUF, &buf) != 0) {
            ESP_LOGE(TAG, "failed to queue buffer %d", i);
            return ESP_FAIL;
        }
    }

    if (ioctl(s_fd, VIDIOC_STREAMON, &type) != 0) {
        ESP_LOGE(TAG, "failed to start stream");
        return ESP_FAIL;
    }

    s_pending_index = -1;
    s_state = CAM_STATE_STREAMING;
    ESP_LOGI(TAG, "camera streaming started");
    return ESP_OK;
}

esp_err_t cam_capture(uint8_t **buf, size_t *size)
{
    const int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    ESP_RETURN_ON_FALSE(s_state == CAM_STATE_STREAMING, ESP_ERR_INVALID_STATE,
                        TAG, "camera is not streaming");

    if (s_pending_index >= 0) {
        struct v4l2_buffer requeue = {
            .type   = type,
            .memory = V4L2_MEMORY_MMAP,
            .index  = s_pending_index,
        };
        if (ioctl(s_fd, VIDIOC_QBUF, &requeue) != 0) {
            ESP_LOGE(TAG, "failed to requeue buffer %d", s_pending_index);
            return ESP_FAIL;
        }
        s_pending_index = -1;
    }

    struct v4l2_buffer vbuf = {
        .type   = type,
        .memory = V4L2_MEMORY_MMAP,
    };
    if (ioctl(s_fd, VIDIOC_DQBUF, &vbuf) != 0) {
        ESP_LOGE(TAG, "failed to receive video frame");
        return ESP_FAIL;
    }

    if (vbuf.flags & V4L2_BUF_FLAG_ERROR) {
        if (ioctl(s_fd, VIDIOC_QBUF, &vbuf) != 0) {
            ESP_LOGE(TAG, "failed to requeue errored buffer");
            return ESP_FAIL;
        }
        ESP_LOGE(TAG, "received an errored frame");
        return ESP_ERR_INVALID_STATE;
    }

    s_pending_index = vbuf.index;
    *buf = s_buffers[vbuf.index];
    *size = vbuf.bytesused;
    return ESP_OK;
}

esp_err_t cam_stop(void)
{
    const int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    ESP_RETURN_ON_FALSE(s_state == CAM_STATE_STREAMING, ESP_ERR_INVALID_STATE,
                        TAG, "camera is not streaming");

    if (s_pending_index >= 0) {
        struct v4l2_buffer requeue = {
            .type   = type,
            .memory = V4L2_MEMORY_MMAP,
            .index  = s_pending_index,
        };
        ioctl(s_fd, VIDIOC_QBUF, &requeue);
        s_pending_index = -1;
    }

    if (ioctl(s_fd, VIDIOC_STREAMOFF, &type) != 0) {
        ESP_LOGE(TAG, "failed to stop stream");
        return ESP_FAIL;
    }

    for (int i = 0; i < CAM_BUFFER_COUNT; i++) {
        if (s_buffers[i]) {
            munmap(s_buffers[i], s_buffer_lengths[i]);
            s_buffers[i] = NULL;
            s_buffer_lengths[i] = 0;
        }
    }

    struct v4l2_requestbuffers req = {
        .count  = 0,
        .type   = type,
        .memory = V4L2_MEMORY_MMAP,
    };
    ioctl(s_fd, VIDIOC_REQBUFS, &req);

    s_state = CAM_STATE_INITIALIZED;
    ESP_LOGI(TAG, "camera streaming stopped");
    return ESP_OK;
}

