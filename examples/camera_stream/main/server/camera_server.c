#include "camera_server.h"
#include "camera.h"
#include "server_util.h"

#include <string.h>
#include "esp_log.h"
#include "esp_http_server.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_jpeg_enc.h"

static const char *TAG = "cam_server";

/* ---------- embedded web resource ---------- */

extern const uint8_t camera_html_gz_start[] asm("_binary_camera_html_gz_start");
extern const uint8_t camera_html_gz_end[]   asm("_binary_camera_html_gz_end");

static const server_rsc_t s_root_rsc = {
    .path     = "/",
    .start    = (char *)camera_html_gz_start,
    .end      = (char *)camera_html_gz_end,
    .type     = SERV_TYPE_HTML,
    .encoding = SERV_ENC_GZIP,
};


#ifndef CONFIG_CAMERA_GC0308_DVP_DEFAULT_FMT_YUV422_YUYV_320X240_20FPS
#error "below config only works if you select CONFIG_CAMERA_GC0308_DVP_DEFAULT_FMT_YUV422_YUYV_320X240_20FPS"
#endif

#define CAM_WIDTH      320
#define CAM_HEIGHT     240
#define JPEG_SRC_FMT   JPEG_PIXEL_FORMAT_YCbYCr
#define JPEG_BUF_SIZE  (64 * 1024)

/* ---------- state ---------- */

static httpd_handle_t     s_server;
static SemaphoreHandle_t  s_cam_lock;
static jpeg_enc_handle_t  s_jpeg_enc;

/* ---------- JPEG helpers ---------- */

static esp_err_t jpeg_enc_ensure(void)
{
    if (s_jpeg_enc) {
        return ESP_OK;
    }
    jpeg_enc_config_t cfg = DEFAULT_JPEG_ENC_CONFIG();
    cfg.width        = CAM_WIDTH;
    cfg.height       = CAM_HEIGHT;
    cfg.src_type     = JPEG_SRC_FMT;
    cfg.subsampling  = JPEG_SUBSAMPLE_420;
    cfg.quality      = 40;
    if (jpeg_enc_open(&cfg, &s_jpeg_enc) != JPEG_ERR_OK) {
        ESP_LOGE(TAG, "jpeg enc open failed");
        return ESP_FAIL;
    }
    return ESP_OK;
}

static esp_err_t encode_frame(const uint8_t *raw, size_t raw_size,
                               uint8_t *jpeg_out, size_t jpeg_out_size, size_t *jpeg_size)
{
    esp_err_t ret = jpeg_enc_ensure();
    if (ret != ESP_OK) {
        return ret;
    }
    if (jpeg_enc_process(s_jpeg_enc, raw, (int)raw_size,
                         jpeg_out, (int)jpeg_out_size, (int *)jpeg_size) != JPEG_ERR_OK) {
        ESP_LOGE(TAG, "jpeg encode failed");
        return ESP_FAIL;
    }
    return ESP_OK;
}

/* ---------- URI handlers ---------- */

#define BOUNDARY "frame"

static esp_err_t stream_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "multipart/x-mixed-replace; boundary=" BOUNDARY);
    httpd_resp_set_hdr(req, "Cache-Control", "no-cache");
    httpd_resp_set_hdr(req, "Connection", "keep-alive");

    uint8_t *jpeg_buf = heap_caps_malloc(JPEG_BUF_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!jpeg_buf) {
        ESP_LOGE(TAG, "no memory for jpeg buf");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "out of memory");
        return ESP_FAIL;
    }

    for (;;) {
        uint8_t *raw = NULL;
        size_t raw_size = 0;

        xSemaphoreTake(s_cam_lock, portMAX_DELAY);
        esp_err_t ret = cam_capture(&raw, &raw_size);
        xSemaphoreGive(s_cam_lock);

        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "capture failed: %s", esp_err_to_name(ret));
            break;
        }

        size_t jpeg_size = 0;
        ret = encode_frame(raw, raw_size, jpeg_buf, JPEG_BUF_SIZE, &jpeg_size);
        if (ret != ESP_OK) {
            break;
        }

        /* send one MJPEG part:  --frame\r\nContent-Type: image/jpeg\r\n\r\n<jpeg>\r\n */
        static const char hdr[] =
            "--" BOUNDARY "\r\n"
            "Content-Type: image/jpeg\r\n"
            "Content-Length: ";
        static const char tail[] = "\r\n\r\n";

        char len_str[16];
        snprintf(len_str, sizeof(len_str), "%u", (unsigned)jpeg_size);

        if (httpd_resp_send_chunk(req, hdr, sizeof(hdr) - 1) != ESP_OK ||
            httpd_resp_send_chunk(req, len_str, strlen(len_str)) != ESP_OK ||
            httpd_resp_send_chunk(req, tail, sizeof(tail) - 1) != ESP_OK ||
            httpd_resp_send_chunk(req, (const char *)jpeg_buf, (ssize_t)jpeg_size) != ESP_OK ||
            httpd_resp_send_chunk(req, "\r\n", 2) != ESP_OK) {
            /* client disconnected */
            break;
        }
    }

    /* final chunk to close the stream cleanly */
    httpd_resp_send_chunk(req, NULL, 0);
    heap_caps_free(jpeg_buf);
    ESP_LOGI(TAG, "stream ended");
    return ESP_OK;
}

/* ---------- start / stop ---------- */

esp_err_t camera_server_start(void)
{
    s_cam_lock = xSemaphoreCreateMutex();
    if (!s_cam_lock) {
        return ESP_ERR_NO_MEM;
    }
    esp_err_t ret;
    ret = cam_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "cam_start failed: %s", esp_err_to_name(ret));
        cam_deinit();
        return ret;
    }

    /* start http server */
    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.lru_purge_enable = true;
    cfg.max_open_sockets = 2;
    cfg.stack_size       = 8192;

    ret = httpd_start(&s_server, &cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "httpd start failed: %s", esp_err_to_name(ret));
        cam_stop();
        cam_deinit();
        return ret;
    }

    serve_rsc(s_server, &s_root_rsc);

    static const httpd_uri_t stream_uri = {
        .uri    = "/stream",
        .method = HTTP_GET,
        .handler = stream_handler,
    };
    httpd_register_uri_handler(s_server, &stream_uri);

    ESP_LOGI(TAG, "camera server started");
    return ESP_OK;
}

void camera_server_stop(void)
{
    if (s_server) {
        httpd_stop(s_server);
        s_server = NULL;
    }
    if (s_jpeg_enc) {
        jpeg_enc_close(s_jpeg_enc);
        s_jpeg_enc = NULL;
    }
    if (cam_get_state() == CAM_STATE_STREAMING) {
        cam_stop();
    }
    cam_deinit();
    ESP_LOGI(TAG, "camera server stopped");
}
