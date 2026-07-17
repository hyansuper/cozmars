#include "server_util.h"

static esp_err_t serve_rsc_handler(httpd_req_t *req) {
    const server_rsc_t* rsc = req->user_ctx;
    if(rsc->type) httpd_resp_set_type(req, rsc->type);
    if(rsc->encoding) httpd_resp_set_hdr(req, "Content-Encoding", rsc->encoding);
    httpd_resp_send(req, rsc->start, rsc->end - rsc->start);
    return ESP_OK;
}

void serve_rsc(httpd_handle_t server, const server_rsc_t* rsc) {
	httpd_uri_t httpd_uri = {
		.uri      = rsc->path,
        .method   = HTTP_GET,
        .handler  = serve_rsc_handler,
        .user_ctx = (void *)rsc,
	};
	httpd_register_uri_handler(server, &httpd_uri);
}

void url_decode(char *dst, const char *src)
{
    while (*src) {
        if (*src == '+') {
            *dst++ = ' ';
            src++;
        } else if (*src == '%' && src[1] && src[2]) {
            char hex[3] = {src[1], src[2], 0};
            *dst++ = strtol(hex, NULL, 16);
            src += 3;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = 0;
}
