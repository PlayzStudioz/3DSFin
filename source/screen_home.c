#include "screen_home.h"
#include "jf_api.h"
#include "ui_theme.h"
#include <3ds.h>
#include <string.h>

static JfLibrary libraries[MAX_LIBRARIES];
static int library_count = 0;
static int selected = 0;
static bool loaded = false;

void screen_home_init(AppState *st) {
    library_count = jf_api_get_libraries(st, libraries);
    selected = 0;
    loaded = true;
}

void screen_home_update(AppState *st) {
    hidScanInput();
    u32 down = hidKeysDown();

    if (!loaded || down & KEY_Y) { // Y refreshes
        library_count = jf_api_get_libraries(st, libraries);
    }

    if (library_count == 0) return;

    if (down & KEY_DOWN) selected = (selected + 1) % library_count;
    if (down & KEY_UP)   selected = (selected + library_count - 1) % library_count;

    if (down & KEY_A) {
        JfLibrary *lib = &libraries[selected];
        strncpy(st->current_library_id, lib->id, sizeof(st->current_library_id) - 1);
        strncpy(st->current_library_name, lib->name, sizeof(st->current_library_name) - 1);
        st->library_page = 0;
        st->current_screen = SCREEN_LIBRARY;
    }

    if (down & KEY_START) st->current_screen = SCREEN_EXIT;
}

void screen_home_draw(AppState *st, C2D_TextBuf textbuf) {
    C2D_TargetClear(NULL, C_BG);

    C2D_Text title;
    char header[MAX_NAME_LEN + 32];
    snprintf(header, sizeof(header), "3DSFin  -  %s", st->username[0] ? st->username : "");
    C2D_TextParse(&title, textbuf, header);
    C2D_TextOptimize(&title);
    C2D_DrawText(&title, C2D_WithColor, PAD_LG, PAD_LG, 0.0f, 0.7f, 0.7f, C_ACCENT_MAIN);

    if (library_count == 0) {
        C2D_Text empty;
        C2D_TextParse(&empty, textbuf, "No libraries found. Press Y to refresh.");
        C2D_TextOptimize(&empty);
        C2D_DrawText(&empty, C2D_WithColor, PAD_LG, 60.0f, 0.0f, 0.5f, 0.5f, C_TEXT_MUTED);
        return;
    }

    float y = 56.0f;
    for (int i = 0; i < library_count; i++) {
        ui_draw_card(PAD_LG, y, TOP_W - PAD_LG * 2, 32.0f, i == selected);
        C2D_Text label;
        C2D_TextParse(&label, textbuf, libraries[i].name);
        C2D_TextOptimize(&label);
        C2D_DrawText(&label, C2D_WithColor, PAD_LG + PAD_MD, y + 8.0f, 0.0f, 0.5f, 0.5f, C_TEXT_PRIMARY);
        y += 32.0f + PAD_SM;
    }
}
