#include "jf_http.h"
#include <3ds.h>
#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// 3DS requires an explicit, page-aligned buffer for the sockets service.
#define SOC_ALIGN       0x1000
#define SOC_BUFFERSIZE  0x100000
static u32 *soc_buffer = NULL;

typedef struct { char *data; size_t len; } GrowBuf;

static size_t write_cb(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t real = size * nmemb;
    GrowBuf *gb = (GrowBuf *)userp;
    char *n = realloc(gb->data, gb->len + real + 1);
    if (!n) return 0; // signals error to curl
    gb->data = n;
    memcpy(gb->data + gb->len, contents, real);
    gb->len += real;
    gb->data[gb->len] = 0;
    return real;
}

bool jf_http_init(void) {
    soc_buffer = (u32 *)memalign(SOC_ALIGN, SOC_BUFFERSIZE);
    if (!soc_buffer) return false;
    if (socInit(soc_buffer, SOC_BUFFERSIZE) != 0) return false;
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != 0) return false;
    return true;
}

void jf_http_shutdown(void) {
    curl_global_cleanup();
    socExit();
    if (soc_buffer) { free(soc_buffer); soc_buffer = NULL; }
}

static JfHttpResponse do_request(const char *url, const char *const *extra_headers,
                                  const char *body, int body_len, bool is_post) {
    JfHttpResponse resp = { .data = NULL, .len = 0, .status = 0 };
    CURL *curl = curl_easy_init();
    if (!curl) return resp;

    GrowBuf gb = { .data = malloc(1), .len = 0 };
    gb.data[0] = 0;

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    if (extra_headers) {
        for (int i = 0; extra_headers[i] != NULL; i++)
            headers = curl_slist_append(headers, extra_headers[i]);
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &gb);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    // Jellyfin servers are typically self-hosted with self-signed certs on
    // local networks; relax verification. Revisit if you expose over WAN.
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    if (is_post) {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body ? body : "");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, body ? body_len : 0);
    }

    CURLcode res = curl_easy_perform(curl);
    if (res == CURLE_OK) {
        long code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
        resp.status = (int)code;
        resp.data = gb.data;
        resp.len = (int)gb.len;
    } else {
        free(gb.data);
        resp.data = NULL;
        resp.len = 0;
        resp.status = 0; // caller treats 0 as "network/transport failure"
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return resp;
}

JfHttpResponse jf_http_get(const char *url, const char *const *extra_headers) {
    return do_request(url, extra_headers, NULL, 0, false);
}

JfHttpResponse jf_http_post(const char *url, const char *const *extra_headers,
                             const char *body, int body_len) {
    return do_request(url, extra_headers, body, body_len, true);
}

void jf_http_free(JfHttpResponse *r) {
    if (r->data) free(r->data);
    r->data = NULL;
    r->len = 0;
}
