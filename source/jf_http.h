#pragma once
#include <stdbool.h>

// Thin wrapper around libcurl (3ds-curl portlib) for the handful of request
// shapes the Jellyfin API needs. Not a general HTTP client.

typedef struct {
    char *data;     // heap-allocated response body, caller must jf_http_free()
    int   len;
    int   status;   // HTTP status code, 0 if the request never completed
} JfHttpResponse;

// Call once at startup (initializes libcurl + 3DS sockets/soc service) and
// once at shutdown.
bool jf_http_init(void);
void jf_http_shutdown(void);

// `extra_headers` is a NULL-terminated array of "Key: Value" strings, or NULL.
JfHttpResponse jf_http_get(const char *url, const char *const *extra_headers);
JfHttpResponse jf_http_post(const char *url, const char *const *extra_headers,
                             const char *body, int body_len);

void jf_http_free(JfHttpResponse *r);
