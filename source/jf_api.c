#include "jf_api.h"
#include "jf_http.h"
#include "jf_json.h"
#include <stdio.h>
#include <string.h>

// Every Jellyfin request identifies the calling app via this header. Server
// admins can see "3DSFin" in their dashboard's active devices list.
#define DEVICE_NAME   "Nintendo 3DS"
#define DEVICE_ID     "3dsfin-homebrew-001"
#define APP_NAME      "3DSFin"
#define APP_VERSION   "0.1.0"

static void build_auth_header(const AppState *st, char *out, int out_size) {
    if (st->access_token[0]) {
        snprintf(out, out_size,
            "X-Emby-Authorization: MediaBrowser Client=\"%s\", Device=\"%s\", "
            "DeviceId=\"%s\", Version=\"%s\", Token=\"%s\"",
            APP_NAME, DEVICE_NAME, DEVICE_ID, APP_VERSION, st->access_token);
    } else {
        snprintf(out, out_size,
            "X-Emby-Authorization: MediaBrowser Client=\"%s\", Device=\"%s\", "
            "DeviceId=\"%s\", Version=\"%s\"",
            APP_NAME, DEVICE_NAME, DEVICE_ID, APP_VERSION);
    }
}

bool jf_api_test_connection(const char *server_url) {
    char url[MAX_URL_LEN];
    snprintf(url, sizeof(url), "%s/System/Info/Public", server_url);
    JfHttpResponse r = jf_http_get(url, NULL);
    bool ok = (r.status == 200);
    jf_http_free(&r);
    return ok;
}

bool jf_api_login(AppState *st, const char *server_url, const char *username, const char *password) {
    strncpy(st->server_url, server_url, sizeof(st->server_url) - 1);

    char auth_header[384];
    build_auth_header(st, auth_header, sizeof(auth_header)); // no token yet
    const char *headers[] = { auth_header, NULL };

    // Naive JSON body -- username/password are not escaped for embedded quotes.
    // Fine for a v0.1 login form; harden if you expect exotic passwords.
    char body[512];
    snprintf(body, sizeof(body), "{\"Username\":\"%s\",\"Pw\":\"%s\"}", username, password);

    char url[MAX_URL_LEN];
    snprintf(url, sizeof(url), "%s/Users/AuthenticateByName", server_url);

    JfHttpResponse r = jf_http_post(url, headers, body, (int)strlen(body));
    if (r.status != 200 || !r.data) { jf_http_free(&r); return false; }

    char arena[8192];
    JfValue *root = jf_parse(r.data, r.len, arena, sizeof(arena));
    bool ok = false;
    if (root) {
        jf_get_string(root, "AccessToken", st->access_token, sizeof(st->access_token));
        JfValue *user = jf_get(root, "User");
        if (user) {
            jf_get_string(user, "Id", st->user_id, sizeof(st->user_id));
            jf_get_string(user, "Name", st->username, sizeof(st->username));
        }
        ok = st->access_token[0] != 0 && st->user_id[0] != 0;
    }
    jf_http_free(&r);
    st->logged_in = ok;
    return ok;
}

int jf_api_get_libraries(AppState *st, JfLibrary *out) {
    char auth_header[384];
    build_auth_header(st, auth_header, sizeof(auth_header));
    const char *headers[] = { auth_header, NULL };

    char url[MAX_URL_LEN];
    snprintf(url, sizeof(url), "%s/Users/%s/Views", st->server_url, st->user_id);

    JfHttpResponse r = jf_http_get(url, headers);
    if (r.status != 200 || !r.data) { jf_http_free(&r); return 0; }

    static char arena[32768]; // static: too big for the 3DS's small default stack
    JfValue *root = jf_parse(r.data, r.len, arena, sizeof(arena));
    int count = 0;
    if (root) {
        JfValue *items = jf_get(root, "Items");
        int n = jf_array_len(items);
        for (int i = 0; i < n && count < MAX_LIBRARIES; i++) {
            JfValue *it = jf_index(items, i);
            jf_get_string(it, "Id", out[count].id, sizeof(out[count].id));
            jf_get_string(it, "Name", out[count].name, sizeof(out[count].name));
            jf_get_string(it, "CollectionType", out[count].collection_type, sizeof(out[count].collection_type));
            count++;
        }
    }
    jf_http_free(&r);
    return count;
}

int jf_api_get_items(AppState *st, const char *library_id, int page, JfItemSummary *out) {
    char auth_header[384];
    build_auth_header(st, auth_header, sizeof(auth_header));
    const char *headers[] = { auth_header, NULL };

    int start = page * MAX_ITEMS_PER_PAGE;
    char url[MAX_URL_LEN];
    snprintf(url, sizeof(url),
        "%s/Users/%s/Items?ParentId=%s&StartIndex=%d&Limit=%d&SortBy=SortName"
        "&Fields=PrimaryImageAspectRatio",
        st->server_url, st->user_id, library_id, start, MAX_ITEMS_PER_PAGE);

    JfHttpResponse r = jf_http_get(url, headers);
    if (r.status != 200 || !r.data) { jf_http_free(&r); return 0; }

    static char arena[65536];
    JfValue *root = jf_parse(r.data, r.len, arena, sizeof(arena));
    int count = 0;
    if (root) {
        JfValue *items = jf_get(root, "Items");
        int n = jf_array_len(items);
        for (int i = 0; i < n && count < MAX_ITEMS_PER_PAGE; i++) {
            JfValue *it = jf_index(items, i);
            jf_get_string(it, "Id", out[count].id, sizeof(out[count].id));
            jf_get_string(it, "Name", out[count].name, sizeof(out[count].name));
            JfValue *img = jf_get(it, "ImageTags");
            char tag[MAX_USERID_LEN] = {0};
            if (img) jf_get_string(img, "Primary", tag, sizeof(tag));
            strncpy(out[count].image_tag, tag, sizeof(out[count].image_tag) - 1);
            out[count].has_image = tag[0] != 0;
            count++;
        }
    }
    jf_http_free(&r);
    return count;
}

bool jf_api_get_item_details(AppState *st, const char *item_id) {
    char auth_header[384];
    build_auth_header(st, auth_header, sizeof(auth_header));
    const char *headers[] = { auth_header, NULL };

    char url[MAX_URL_LEN];
    snprintf(url, sizeof(url), "%s/Users/%s/Items/%s", st->server_url, st->user_id, item_id);

    JfHttpResponse r = jf_http_get(url, headers);
    if (r.status != 200 || !r.data) { jf_http_free(&r); return false; }

    static char arena[65536];
    JfValue *root = jf_parse(r.data, r.len, arena, sizeof(arena));
    if (!root) { jf_http_free(&r); return false; }

    strncpy(st->current_item_id, item_id, sizeof(st->current_item_id) - 1);
    jf_get_string(root, "Name", st->current_item_name, sizeof(st->current_item_name));
    jf_get_string(root, "Overview", st->current_item_overview, sizeof(st->current_item_overview));

    // MediaSources[0].Id is what the streaming endpoints expect.
    JfValue *sources = jf_get(root, "MediaSources");
    JfValue *src0 = jf_index(sources, 0);
    if (src0) jf_get_string(src0, "Id", st->current_media_source_id, sizeof(st->current_media_source_id));

    // Subtitle track [0] is always synthetic "None" -- burning in subtitles
    // is optional, most playback has none selected.
    st->subtitle_track_count = 0;
    strncpy(st->subtitle_tracks[0].label, "None", sizeof(st->subtitle_tracks[0].label) - 1);
    st->subtitle_tracks[0].index = -1;
    st->subtitle_tracks[0].is_default = true;
    st->subtitle_track_count = 1;
    st->selected_subtitle_track = 0;

    st->audio_track_count = 0;
    st->selected_audio_track = 0;

    JfValue *streams = src0 ? jf_get(src0, "MediaStreams") : NULL;
    int n = jf_array_len(streams);
    for (int i = 0; i < n; i++) {
        JfValue *stream = jf_index(streams, i);
        char type[16] = {0};
        jf_get_string(stream, "Type", type, sizeof(type));
        int idx = (int)jf_get_number(stream, "Index", -1);
        char lang[16] = {0}, title[MAX_NAME_LEN] = {0}, codec[16] = {0};
        jf_get_string(stream, "Language", lang, sizeof(lang));
        jf_get_string(stream, "DisplayTitle", title, sizeof(title));
        jf_get_string(stream, "Codec", codec, sizeof(codec));
        bool is_default = jf_get_bool(stream, "IsDefault", false);

        const char *label = title[0] ? title : (lang[0] ? lang : codec);

        if (strcmp(type, "Audio") == 0 && st->audio_track_count < MAX_TRACKS) {
            TrackOption *t = &st->audio_tracks[st->audio_track_count];
            t->index = idx;
            t->is_default = is_default;
            strncpy(t->label, label[0] ? label : "Audio track", sizeof(t->label) - 1);
            if (is_default) st->selected_audio_track = st->audio_track_count;
            st->audio_track_count++;
        } else if (strcmp(type, "Subtitle") == 0 && st->subtitle_track_count < MAX_TRACKS) {
            TrackOption *t = &st->subtitle_tracks[st->subtitle_track_count];
            t->index = idx;
            t->is_default = is_default;
            strncpy(t->label, label[0] ? label : "Subtitle track", sizeof(t->label) - 1);
            if (is_default) st->selected_subtitle_track = st->subtitle_track_count;
            st->subtitle_track_count++;
        }
    }

    jf_http_free(&r);
    return true;
}

// Resolution/bitrate caps per quality preset. Conservative on purpose --
// see README "How playback works". Tune once real hardware testing starts.
static void quality_params(VideoQuality q, int *w, int *h, int *bitrate) {
    switch (q) {
        case QUALITY_POTATO: *w = 320; *h = 180; *bitrate = 250000;  break;
        case QUALITY_MEDIUM: *w = 480; *h = 270; *bitrate = 900000;  break;
        case QUALITY_LOW:
        default:             *w = 400; *h = 224; *bitrate = 500000; break;
    }
}

void jf_api_build_playback_url(AppState *st) {
    int w, h, bitrate;
    quality_params(st->quality, &w, &h, &bitrate);

    int audio_idx = (st->audio_track_count > 0)
        ? st->audio_tracks[st->selected_audio_track].index : -1;
    int sub_idx = (st->subtitle_track_count > 0)
        ? st->subtitle_tracks[st->selected_subtitle_track].index : -1;

    // Server-side work happens here: fixed baseline H.264 + AAC container the
    // 3DS can decode, chosen audio track remuxed/transcoded to stereo, chosen
    // subtitle track burned into the video (SubtitleMethod=Encode) so the
    // device never has to render text over frames itself.
    int n = snprintf(st->playback_url, sizeof(st->playback_url),
        "%s/Videos/%s/stream.mp4?"
        "MediaSourceId=%s"
        "&VideoCodec=h264&AudioCodec=aac"
        "&MaxWidth=%d&MaxHeight=%d&VideoBitRate=%d"
        "&AudioChannels=2"
        "&Profile=baseline&Level=30"
        "&AudioStreamIndex=%d"
        "&static=false",
        st->server_url, st->current_item_id, st->current_media_source_id,
        w, h, bitrate, audio_idx);

    if (sub_idx >= 0 && n > 0 && n < (int)sizeof(st->playback_url)) {
        char extra[128];
        snprintf(extra, sizeof(extra), "&SubtitleStreamIndex=%d&SubtitleMethod=Encode", sub_idx);
        strncat(st->playback_url, extra, sizeof(st->playback_url) - strlen(st->playback_url) - 1);
    }

    if (st->access_token[0]) {
        char extra[MAX_TOKEN_LEN + 16];
        snprintf(extra, sizeof(extra), "&api_key=%s", st->access_token);
        strncat(st->playback_url, extra, sizeof(st->playback_url) - strlen(st->playback_url) - 1);
    }
}

void jf_api_build_image_url(AppState *st, const char *item_id, const char *image_tag,
                             int max_width, char *out, int out_size) {
    snprintf(out, out_size, "%s/Items/%s/Images/Primary?maxWidth=%d&tag=%s&quality=90",
        st->server_url, item_id, max_width, image_tag);
}
