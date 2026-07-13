#include <3ds.h>
#include <citro2d.h>
#include <citro3d.h>

#include "app_state.h"
#include "ui_theme.h"
#include "jf_http.h"
#include "screen_login.h"
#include "screen_home.h"
#include "screen_library.h"
#include "screen_details.h"
#include "screen_player.h"

// Screens that use the bottom (touch) screen for their primary controls.
// Everyone else just gets a plain bottom-screen background.
static bool screen_uses_bottom_primary(ScreenId id) {
    return id == SCREEN_LOGIN || id == SCREEN_DETAILS;
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;

    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    C3D_RenderTarget *top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    C3D_RenderTarget *bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    C2D_TextBuf textbuf = C2D_TextBufNew(4096);

    if (!jf_http_init()) {
        // Without networking there's nothing useful to do; still boot to a
        // visible error rather than silently exiting.
    }

    AppState state;
    app_state_init(&state);

    ScreenId last_init_screen = SCREEN_EXIT; // force first-frame init below

    while (aptMainLoop()) {
        hidScanInput();
        u32 down = hidKeysDown();
        if (down & KEY_START && state.current_screen != SCREEN_LOGIN) {
            // START is reserved as "quit" everywhere except the login form,
            // where it's free for text entry flows.
        }

        if (state.current_screen == SCREEN_EXIT) break;

        // Run each screen's init() exactly once when we transition into it --
        // this is where each screen kicks off its API calls.
        if (state.current_screen != last_init_screen) {
            switch (state.current_screen) {
                case SCREEN_LOGIN:   screen_login_init(&state);   break;
                case SCREEN_HOME:    screen_home_init(&state);    break;
                case SCREEN_LIBRARY: screen_library_init(&state); break;
                case SCREEN_DETAILS: screen_details_init(&state);break;
                case SCREEN_PLAYER:  screen_player_init(&state); break;
                default: break;
            }
            last_init_screen = state.current_screen;
        }

        switch (state.current_screen) {
            case SCREEN_LOGIN:   screen_login_update(&state);   break;
            case SCREEN_HOME:    screen_home_update(&state);    break;
            case SCREEN_LIBRARY: screen_library_update(&state); break;
            case SCREEN_DETAILS: screen_details_update(&state); break;
            case SCREEN_PLAYER:  screen_player_update(&state); break;
            default: break;
        }

        C2D_TextBufClear(textbuf);
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

        // Login and Details are laid out for the 320x240 bottom (touch)
        // screen since their fields/rows are meant to be reachable by touch
        // later; everything else (Home, Library, Player) is laid out for
        // the wider 400x240 top screen. Only one target gets the screen's
        // real content per frame -- the other gets a plain themed fill.
        bool primary_is_bottom = screen_uses_bottom_primary(state.current_screen);

        C2D_TargetClear(top, C_BG);
        C2D_SceneBegin(top);
        if (!primary_is_bottom) {
            switch (state.current_screen) {
                case SCREEN_HOME:    screen_home_draw(&state, textbuf);    break;
                case SCREEN_LIBRARY: screen_library_draw(&state, textbuf); break;
                case SCREEN_PLAYER:  screen_player_draw(&state, textbuf);  break;
                default: break;
            }
        }

        C2D_TargetClear(bottom, C_BG);
        C2D_SceneBegin(bottom);
        if (primary_is_bottom) {
            switch (state.current_screen) {
                case SCREEN_LOGIN:   screen_login_draw(&state, textbuf);   break;
                case SCREEN_DETAILS: screen_details_draw(&state, textbuf); break;
                default: break;
            }
        }

        C3D_FrameEnd(0);
    }

    jf_http_shutdown();
    C2D_TextBufDelete(textbuf);
    C2D_Fini();
    C3D_Fini();
    gfxExit();
    return 0;
}
