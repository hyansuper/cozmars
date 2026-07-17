#pragma once

#include "esp_err.h"
#include <esp_http_server.h>

typedef struct {
	char* path;
	char* start;
	char* end;
	char* encoding;
	char* type;
} server_rsc_t;

#define SERV_TYPE_JAVASCRIPT "application/javascript"
#define SERV_TYPE_JSON "application/json"
#define SERV_TYPE_ICO "image/x-icon"
#define SERV_TYPE_PNG "image/png"
#define SERV_TYPE_JPG "image/jpeg"
#define SERV_TYPE_HTML "text/html"

#define SERV_ENC_GZIP "gzip"


void serve_rsc(httpd_handle_t server, const server_rsc_t* rsc);

// This function performs URL percent-decoding (also called URL decoding / form decoding).
// it can perform in-place decode
void url_decode(char *dst, const char *src);