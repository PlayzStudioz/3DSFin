#pragma once
#include <3ds.h>

#define MAX_URL_LEN      256
#define MAX_TOKEN_LEN     64
#define MAX_USERID_LEN     40
#define MAX_NAME_LEN      128
#define MAX_TRACKS         16

// Which screen is currently active. main.c switches on this each frame.
typedef enum {
    SCREEN_LOGIN = 0,
    SCREEN_HOME,
    SCREEN_LIBRARY,
    SCREEN_DETAILS,
    SCREEN_PLAYER,
    SCREEN_EXIT
} ScreenId;

// One audio or subtitle track option, as surfaced by Jellyfin's MediaStreams.
typedef struct {
    int   index;                 // Jellyfin stream index, used in playback URL
    char  label[MAX_NAME_LEN];   // e.g. "English (AAC 5.1)" or "English (SRT)"
    bool  is_default;
} TrackOption;

// Playback video quality presets. Kept conservative -- the 3DS screen is
// 400x240 top / 320x240 bottom, and the CPU has no hardware H.264 decoder.
typedef enum {
    QUALITY_POTATO = 0,  // 320x180 @ ~250kbps -- safest, use for testing
    QUALITY_LOW,          // 400x224 @ ~500kbps -- default
    QUALITY_MEDIUM        // 480x270 @ ~900kbps -- may not hold framerate
} VideoQuality;

// Global session/config, allocated once in main() and passed around by pointer.
typedef struct {
    // --- server / auth ---
    char server_url[MAX_URL_LEN];      // e.g. "http://192.168.1.50:8096"
    char access_token[MAX_TOKEN_LEN];
    char user_id[MAX_USERID_LEN];
    char username[MAX_NAME_LEN];
    bool logged_in;

    // --- settings ---
    VideoQuality quality;

    // --- navigation ---
    ScreenId current_screen;
    ScreenId previous_screen;

    // --- currently browsed library ---
    char current_library_id[MAX_USERID_LEN];
    char current_library_name[MAX_NAME_LEN];
    int  library_page;

    // --- currently selected item (movie/episode/etc) ---
    char current_item_id[MAX_USERID_LEN];
    char current_item_name[MAX_NAME_LEN];
    char current_item_overview[1024];
    char current_media_source_id[MAX_USERID_LEN];

    TrackOption audio_tracks[MAX_TRACKS];
    int audio_track_count;
    int selected_audio_track;   // index into audio_tracks[], not the Jellyfin index

    TrackOption subtitle_tracks[MAX_TRACKS]; // includes a synthetic "None" at [0]
    int subtitle_track_count;
    int selected_subtitle_track;

    char playback_url[MAX_URL_LEN * 2];
} AppState;

void app_state_init(AppState *st);
