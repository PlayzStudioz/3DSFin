#pragma once
#include "app_state.h"
#include <stdbool.h>

#define MAX_LIBRARIES 16
#define MAX_ITEMS_PER_PAGE 24

typedef struct {
    char id[MAX_USERID_LEN];
    char name[MAX_NAME_LEN];
    char collection_type[32]; // "movies", "tvshows", "music", ...
} JfLibrary;

typedef struct {
    char id[MAX_USERID_LEN];
    char name[MAX_NAME_LEN];
    char image_tag[MAX_USERID_LEN]; // for building poster URLs; empty if no image
    bool has_image;
} JfItemSummary;

// --- Auth ---
// Fills st->access_token / user_id / username on success.
bool jf_api_login(AppState *st, const char *server_url, const char *username, const char *password);
bool jf_api_test_connection(const char *server_url);

// --- Browsing ---
// Returns count written into `out` (capped at MAX_LIBRARIES).
int jf_api_get_libraries(AppState *st, JfLibrary *out);
// Returns count written into `out` (capped at MAX_ITEMS_PER_PAGE).
int jf_api_get_items(AppState *st, const char *library_id, int page, JfItemSummary *out);

// --- Item details ---
// Populates st->current_item_* fields and st->audio_tracks / subtitle_tracks.
bool jf_api_get_item_details(AppState *st, const char *item_id);

// Builds the final st->playback_url from the currently selected item + tracks
// + st->quality, using server-side transcoding (audio remux, burned-in subs).
void jf_api_build_playback_url(AppState *st);

// Builds an image URL for a poster thumbnail (Primary image, given a max width).
void jf_api_build_image_url(AppState *st, const char *item_id, const char *image_tag,
                             int max_width, char *out, int out_size);
