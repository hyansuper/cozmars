#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/timers.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_http_server.h"
#include "nvs_flash.h"
#include "esp_mac.h"
#include "dns_server.h"
#include "lwip/inet.h"
#include "wifi.h"
#include "server_util.h"


// #define USE_DHCP_CAPTIVEPORTAL


extern const uint8_t wifi_prov_html_gz_start[] asm("_binary_wifi_prov_html_gz_start");
extern const uint8_t wifi_prov_html_gz_end[]   asm("_binary_wifi_prov_html_gz_end");

static const char *TAG = "wifi";

#define WIFI_CONNECTED_BIT   BIT0
#define WIFI_FAIL_BIT        BIT1
#define WIFI_PROVISIONED_BIT BIT2

#define INITIAL_MAX_RETRY   3
#define PROV_MAX_RETRY      2
#define STA_WAIT_TIMEOUT_MS 15000
#define PROV_WAIT_TIMEOUT_MS 8000
#define AP_MAX_CONN         2

static EventGroupHandle_t event_group;
static int retry_count = INITIAL_MAX_RETRY;
static char hostname[] = "Cozmars-0000";
static bool sta_init_conn = false;

static void reconnect_timer_cb(TimerHandle_t t)
{
    wifi_mode_t mode;
    if (esp_wifi_get_mode(&mode) != ESP_OK || mode != WIFI_MODE_STA) {
        return;
    }
    ESP_LOGI(TAG, "reconnect backoff expired, attempting reconnect");
    esp_wifi_connect();
}

static void event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    static TimerHandle_t reconnect_timer;
    static uint32_t reconnect_delay_ms;

    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START && !sta_init_conn) {
            esp_wifi_connect();
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        if (--retry_count > 0) {
            ESP_LOGI(TAG, "reconnect attempt, retries left=%d", retry_count);
            esp_wifi_connect();
        } else {
            if (!sta_init_conn) {
                xEventGroupSetBits(event_group, WIFI_FAIL_BIT);
                return;
            }
            if (!reconnect_timer) {
                reconnect_delay_ms = 15000;
                reconnect_timer = xTimerCreate("reconnect", pdMS_TO_TICKS(reconnect_delay_ms),
                                               pdFALSE, NULL, reconnect_timer_cb);
            } else {
                reconnect_delay_ms *= 2;
                if (reconnect_delay_ms > 600000) // 10 min
                    reconnect_delay_ms = 600000;
                xTimerChangePeriod(reconnect_timer, pdMS_TO_TICKS(reconnect_delay_ms), 0);
            }
            if(reconnect_timer==NULL || pdFAIL==xTimerStart(reconnect_timer, 0)) {
                ESP_LOGE(TAG, "failed to start reconnect timer. restarting...");
                esp_restart();
            }
        }
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *ev = (ip_event_got_ip_t *)data;
        ESP_LOGI(TAG, "got IP: " IPSTR, IP2STR(&ev->ip_info.ip));
        if (reconnect_timer && pdPASS==xTimerDelete(reconnect_timer, 0)) {
            reconnect_timer = NULL;
        }
        sta_init_conn = true;
        retry_count = 2;
        xEventGroupSetBits(event_group, WIFI_CONNECTED_BIT);
    }
}

static esp_err_t http_get_scan_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");

    wifi_scan_config_t scan_cfg = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = false,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .scan_time.active.min = 100,
        .scan_time.active.max = 300,
    };

    esp_wifi_scan_start(&scan_cfg, true);

    uint16_t ap_count = 0;
    esp_wifi_scan_get_ap_num(&ap_count);
    wifi_ap_record_t *recs = calloc(ap_count, sizeof(wifi_ap_record_t));
    if (recs) {
        esp_wifi_scan_get_ap_records(&ap_count, recs);
    }

    size_t alloc = 128 + ap_count * 120;
    char *json = malloc(alloc);
    if (!json) {
        httpd_resp_sendstr(req, "{\"aps\":[]}");
        goto end;
    }

    char *p = json;
    p += sprintf(p, "{\"aps\":[");
    for (int i = 0; i < ap_count; i++) {
        if (i > 0) *p++ = ',';
        char esc[64];
        const char *s = (const char *)recs[i].ssid;
        char *d = esc;
        while (*s && d - esc < (int)sizeof(esc) - 3) {
            if (*s == '"' || *s == '\\') *d++ = '\\';
            *d++ = *s++;
        }
        *d = 0;
        const char *auth = "?";
        switch (recs[i].authmode) {
            case WIFI_AUTH_OPEN: auth = "open"; break;
            case WIFI_AUTH_WEP: auth = "wep"; break;
            case WIFI_AUTH_WPA_PSK: auth = "wpa"; break;
            case WIFI_AUTH_WPA2_PSK: auth = "wpa2"; break;
            case WIFI_AUTH_WPA_WPA2_PSK: auth = "wpa_wpa2"; break;
            case WIFI_AUTH_WPA3_PSK: auth = "wpa3"; break;
            default: break;
        }
        p += sprintf(p, "{\"ssid\":\"%s\",\"rssi\":%d,\"auth\":\"%s\"}",
                     esc, recs[i].rssi, auth);
    }
    p += sprintf(p, "]}");

    httpd_resp_sendstr(req, json);
    free(json);
end:
    free(recs);
    return ESP_OK;
}

static void locate_query(const char* buf, const char* query, char** start, char** end) 
{  
    *start = strstr(buf, query);
    if(*start) {
        *start += strlen(query);
        *end = strchr(*start, '&');
        if (*end == NULL) *end = *start + strlen(*start);
    } else {
        *end = NULL;
    }
}

static esp_err_t http_post_connect_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");
    char buf[384];
    int len = httpd_req_recv(req, buf, sizeof(buf)-1);
    if (len <= 0) {
        httpd_resp_sendstr(req, "{\"status\":\"error\",\"message\":\"no data\"}");
        return ESP_OK;
    }
    buf[len] = 0;

    char* ssid, *ssid_end, *pw, *pw_end;
    locate_query(buf, "ssid=", &ssid, &ssid_end);
    if (ssid == NULL || ssid_end-ssid == 0) {
        httpd_resp_sendstr(req, "{\"status\":\"error\",\"message\":\"missing ssid\"}");
        return ESP_OK;
    }

    locate_query(buf, "password=", &pw, &pw_end);
    if(ssid_end) *ssid_end = 0;
    if(pw_end) *pw_end = 0;
    url_decode(ssid, ssid);
    url_decode(pw, pw);
    ESP_LOGI(TAG, "connecting to ssid=\"%s\"", ssid);

    wifi_config_t wifi_cfg = {0};
    strncpy((char *)wifi_cfg.sta.ssid, ssid, sizeof(wifi_cfg.sta.ssid) - 1);
    strncpy((char *)wifi_cfg.sta.password, pw?pw:"", sizeof(wifi_cfg.sta.password) - 1);
    // wifi_cfg.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    // esp_wifi_set_storage(WIFI_STORAGE_FLASH);
    esp_err_t err = esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg);
    if (err != ESP_OK) {
        httpd_resp_sendstr(req, "{\"status\":\"error\",\"message\":\"save failed\"}");
        return ESP_OK;
    }

    xEventGroupClearBits(event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);
    retry_count = PROV_MAX_RETRY;
    esp_wifi_disconnect();
    esp_wifi_connect();

    EventBits_t bits = xEventGroupWaitBits(event_group,
                        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                        pdFALSE, pdFALSE, pdMS_TO_TICKS(PROV_WAIT_TIMEOUT_MS));

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "provisioning successful");
        httpd_resp_sendstr(req, "{\"status\":\"ok\"}");
        xEventGroupSetBits(event_group, WIFI_PROVISIONED_BIT);
    } else {
        ESP_LOGW(TAG, "provisioning failed to connect");
        httpd_resp_sendstr(req, "{\"status\":\"error\",\"message\":\"connection failed\"}");
    }

    return ESP_OK;
}

static const server_rsc_t root_html = {
    .path = "/",
    .start = (char *)wifi_prov_html_gz_start,
    .end = (char *)wifi_prov_html_gz_end,
    .type = SERV_TYPE_HTML,
    .encoding = SERV_ENC_GZIP,
};
static const httpd_uri_t scan    = {.uri = "/scan", .method = HTTP_GET, .handler = http_get_scan_handler};
static const httpd_uri_t connect = {.uri = "/connect", .method = HTTP_POST, .handler = http_post_connect_handler};

static esp_err_t http_404_redirect_handler(httpd_req_t *req, httpd_err_code_t err)
{
    httpd_resp_set_status(req, "303 See Other");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_send(req, "Redirect to the captive portal", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static void start_prov_web_server(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.lru_purge_enable = true;
    cfg.max_open_sockets = 4;
    cfg.stack_size = 6144;

    if (httpd_start(&server, &cfg) != ESP_OK) {
        ESP_LOGE(TAG, "web server start failed");
        return;
    }
    serve_rsc(server, &root_html);
    httpd_register_uri_handler(server, &scan);
    httpd_register_uri_handler(server, &connect);
    httpd_register_err_handler(server, HTTPD_404_NOT_FOUND, http_404_redirect_handler);
    ESP_LOGI(TAG, "provision web server started.");
}

esp_err_t wifi_get_ip(char *ip_str, size_t len)
{
    if (!ip_str || len < 16) {
        return ESP_ERR_INVALID_ARG;
    }
    esp_netif_t *sta_netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (!sta_netif) {
        return ESP_ERR_NOT_FOUND;
    }
    esp_netif_ip_info_t ip_info;
    esp_err_t err = esp_netif_get_ip_info(sta_netif, &ip_info);
    if (err != ESP_OK) {
        return err;
    }
    if (ip_info.ip.addr == 0) {
        return ESP_ERR_INVALID_STATE;
    }
    esp_ip4addr_ntoa(&ip_info.ip, ip_str, (int)len);
    return ESP_OK;
}

esp_err_t wifi_get_rssi(int8_t *rssi)
{
    if (!rssi) {
        return ESP_ERR_INVALID_ARG;
    }
    wifi_ap_record_t ap_info;
    esp_err_t err = esp_wifi_sta_get_ap_info(&ap_info);
    if (err != ESP_OK) {
        return err;
    }
    *rssi = ap_info.rssi;
    return ESP_OK;
}

esp_err_t wifi_init(void)
{
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_BASE);
    snprintf(hostname, sizeof(hostname), "Cozmars-%02X%02X", mac[4], mac[5]);   

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    event_group = xEventGroupCreate();

    esp_netif_t* sta_netif = esp_netif_create_default_wifi_sta();
    esp_netif_set_hostname(sta_netif, hostname);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    ESP_ERROR_CHECK(esp_wifi_start());
    
    EventBits_t bits = xEventGroupWaitBits(event_group,
                        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                        pdFALSE, pdFALSE, pdMS_TO_TICKS(STA_WAIT_TIMEOUT_MS));

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "WiFi connected");
        return ESP_OK;
    }

    ESP_LOGW(TAG, "STA connection failed, starting AP provisioning");

    ESP_ERROR_CHECK(esp_wifi_stop());

    wifi_config_t ap_cfg = {
        .ap = {
            .ssid_len = strlen(hostname),
            .max_connection = AP_MAX_CONN,
            .authmode = WIFI_AUTH_OPEN,
            .pmf_cfg.required = false,
        },
    };
    strcpy((char *)ap_cfg.ap.ssid, hostname); 

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    esp_netif_t *ap_netif = esp_netif_create_default_wifi_ap();
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_cfg));
    esp_netif_set_hostname(ap_netif, hostname);
    ESP_ERROR_CHECK(esp_wifi_start());

    esp_netif_ip_info_t ip;
    esp_netif_get_ip_info(ap_netif, &ip);
    ESP_LOGI(TAG, "AP \"%s\" on " IPSTR, ap_cfg.ap.ssid, IP2STR(&ip.ip));

    start_prov_web_server();

    dns_server_config_t dns_cfg = DNS_SERVER_CONFIG_SINGLE("*", "WIFI_AP_DEF");
    start_dns_server(&dns_cfg);

#ifdef USE_DHCP_CAPTIVEPORTAL
    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_AP_DEF"), &ip_info);
    char ip_addr[16];
    inet_ntoa_r(ip_info.ip.addr, ip_addr, 16);
    char *uri = malloc(32);
    snprintf(uri, 32, "http://%s", ip_addr);
    esp_netif_t *dhcp_netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_dhcps_stop(dhcp_netif));
    ESP_ERROR_CHECK(esp_netif_dhcps_option(dhcp_netif, ESP_NETIF_OP_SET, ESP_NETIF_CAPTIVEPORTAL_URI, uri, strlen(uri)));
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_dhcps_start(dhcp_netif));
    free(uri);
#endif

    xEventGroupWaitBits(event_group, WIFI_PROVISIONED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

    vTaskDelay(pdMS_TO_TICKS(500));
    ESP_LOGI(TAG, "provisioned, restarting");
    esp_restart();
    return ESP_OK;
}
