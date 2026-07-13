#include "screen_player.h"
#include "ui_theme.h"
#include <3ds.h>
#include <string.h>

// This screen is intentionally a stub. Real video playback needs:
//   1. Pulling the MP4/fMP4 fragments from st->playback_url over HTTP
//      (chunked, since you don't want the whole file in RAM).
//   2. Demuxing (minimal MP4 box parser -- ffmpeg's demuxer is overkill
//      for a single well-known container from your own transcode profile).
//   3. Decoding H.264 baseline frames. There is no 3DS hardware H.264
//      decoder exposed to homebrew; software decode (e.g. a stripped-down
//      openh264 or a from-scratch baseline-profile decoder) at 400x224 is
//      the realistic path, and framerate will be tight.
//   4. Decoding AAC audio and feeding it to ndsp for output.
//   5. Uploading decoded frames into a citro3d texture and drawing it as a
//      textured quad on the top screen, timed against the audio clock.
// That's a real project on its own -- worth its own milestone rather than a
// guessed-at implementation here. This stub proves the URL/flow is correct
// and gives you a place to wire the decoder in.

void screen_player_init(AppState *st) {
    (void)st;
}

void screen_player_update(AppState *st) {
    hidScanInput();
    u32 down = hidKeysDown();
    if (down & KEY_B) st->current_screen = SCREEN_DETAILS;
}

void screen_player_draw(AppState *st, C2D_TextBuf textbuf) {
    C2D_TargetClear(NULL, C_BG);

    C2D_Text title;
    C2D_TextParse(&title, textbuf, "Playback not implemented yet");
    C2D_TextOptimize(&title);
    C2D_DrawText(&title, C2D_WithColor, PAD_LG, PAD_LG, 0.0f, 0.55f, 0.55f, C_ERROR);

    C2D_Text url;
    C2D_TextParse(&url, textbuf, st->playback_url);
    C2D_TextOptimize(&url);
    C2D_DrawText(&url, C2D_WithColor, PAD_LG, 48.0f, 0.0f, 0.32f, 0.32f, C_TEXT_MUTED);

    C2D_Text hint;
    C2D_TextParse(&hint, textbuf, "This is the exact URL 3DSFin would request.\n"
                                   "Paste it into VLC on a PC to confirm your\n"
                                   "server-side track selection is correct.\n\nB: back");
    C2D_TextOptimize(&hint);
    C2D_DrawText(&hint, C2D_WithColor, PAD_LG, 100.0f, 0.0f, 0.45f, 0.45f, C_TEXT_PRIMARY);
}
