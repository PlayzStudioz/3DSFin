#include "screen_details.h"
#include "jf_api.h"
#include "ui_theme.h"
#include <3ds.h>
#include <string.h>
#include <stdio.h>

typedef enum { ROW_AUDIO = 0, ROW_SUBTITLE, ROW_PLAY, ROW_COUNT } Row;
static Row focused_row = ROW_AUDIO;
static bool details_loaded = false;

void screen_details_init(AppState *st) {
    details_loaded = jf_api_get_item_details(st, st->current_item_id);
    focused_row = ROW_AUDIO;
}

void screen_details_update(AppState *st) {
    hidScanInput();
    u32 down = hidKeysDown();

    if (down & KEY_B) { st->current_screen = SCREEN_LIBRARY; return; }
    if (!details_loaded) return;

    if (down & KEY_DOWN) focused_row = (Row)((focused_row + 1) % ROW_COUNT);
    if (down & KEY_UP)   focused_row = (Row)((focused_row + ROW_COUNT - 1) % ROW_COUNT);

    // Left/Right cycle the option within the focused row -- this is the
    // actual language/subtitle switch the user drives before playback.
    if (focused_row == ROW_AUDIO && st->audio_track_count > 0) {
        if (down & KEY_RIGHT) st->selected_audio_track = (st->selected_audio_track + 1) % st->audio_track_count;
        if (down & KEY_LEFT)  st->selected_audio_track = (st->selected_audio_track + st->audio_track_count - 1) % st->audio_track_count;
    }
    if (focused_row == ROW_SUBTITLE && st->subtitle_track_count > 0) {
        if (down & KEY_RIGHT) st->selected_subtitle_track = (st->selected_subtitle_track + 1) % st->subtitle_track_count;
        if (down & KEY_LEFT)  st->selected_subtitle_track = (st->selected_subtitle_track + st->subtitle_track_count - 1) % st->subtitle_track_count;
    }

    if (down & KEY_A && focused_row == ROW_PLAY) {
        jf_api_build_playback_url(st);
        st->current_screen = SCREEN_PLAYER;
    }
}

static void draw_row(float y, const char *label, const char *value, bool focused, C2D_TextBuf textbuf) {
    ui_draw_card(PAD_LG, y, BOT_W - PAD_LG * 2, 36.0f, focused);

    char text[192];
    snprintf(text, sizeof(text), "%s:  < %s >", label, value);
    C2D_Text t;
    C2D_TextParse(&t, textbuf, text);
    C2D_TextOptimize(&t);
    C2D_DrawText(&t, C2D_WithColor, PAD_LG + PAD_MD, y + 10.0f, 0.0f, 0.45f, 0.45f, C_TEXT_PRIMARY);
}

void screen_details_draw(AppState *st, C2D_TextBuf textbuf) {
    C2D_TargetClear(NULL, C_BG);

    if (!details_loaded) {
        C2D_Text err;
        C2D_TextParse(&err, textbuf, "Couldn't load this item.");
        C2D_TextOptimize(&err);
        C2D_DrawText(&err, C2D_WithColor, PAD_LG, PAD_LG, 0.0f, 0.5f, 0.5f, C_ERROR);
        return;
    }

    // This whole screen renders on the 320x240 bottom target (see main.c) --
    // title kept short, full overview/cast is a good candidate for a future
    // "Info" sub-screen on the top display.
    C2D_Text title;
    C2D_TextParse(&title, textbuf, st->current_item_name);
    C2D_TextOptimize(&title);
    C2D_DrawText(&title, C2D_WithColor, PAD_LG, PAD_SM, 0.0f, 0.5f, 0.5f, C_ACCENT_MAIN);

    const char *audio_label = (st->audio_track_count > 0)
        ? st->audio_tracks[st->selected_audio_track].label : "(none available)";
    const char *sub_label = (st->subtitle_track_count > 0)
        ? st->subtitle_tracks[st->selected_subtitle_track].label : "None";

    float y = 40.0f;
    draw_row(y, "Audio", audio_label, focused_row == ROW_AUDIO, textbuf);
    y += 44.0f;
    draw_row(y, "Subtitles", sub_label, focused_row == ROW_SUBTITLE, textbuf);
    y += 44.0f;

    ui_draw_card(PAD_LG, y, BOT_W - PAD_LG * 2, 36.0f, focused_row == ROW_PLAY);
    C2D_Text play;
    C2D_TextParse(&play, textbuf, "> Play  (A)");
    C2D_TextOptimize(&play);
    C2D_DrawText(&play, C2D_WithColor, PAD_LG + PAD_MD, y + 10.0f, 0.0f, 0.5f, 0.5f, C_TEXT_PRIMARY);

    C2D_Text hint;
    C2D_TextParse(&hint, textbuf, "Left/Right to change track, B to go back");
    C2D_TextOptimize(&hint);
    C2D_DrawText(&hint, C2D_WithColor, PAD_LG, BOT_H - 16.0f, 0.0f, 0.35f, 0.35f, C_TEXT_MUTED);
}
