#include "screen_library.h"
#include "jf_api.h"
#include "ui_theme.h"
#include <3ds.h>
#include <string.h>
#include <stdio.h>

static JfItemSummary items[MAX_ITEMS_PER_PAGE];
static int item_count = 0;
static int selected = 0;

static void load_page(AppState *st) {
    item_count = jf_api_get_items(st, st->current_library_id, st->library_page, items);
    selected = 0;
}

void screen_library_init(AppState *st) {
    load_page(st);
}

void screen_library_update(AppState *st) {
    hidScanInput();
    u32 down = hidKeysDown();

    if (down & KEY_B) { st->current_screen = SCREEN_HOME; return; }
    if (item_count == 0) return;

    int cols = GRID_COLS;
    int rows = (item_count + cols - 1) / cols;

    if (down & KEY_RIGHT && (selected + 1) % cols != 0 && selected + 1 < item_count) selected++;
    if (down & KEY_LEFT  && selected % cols != 0) selected--;
    if (down & KEY_DOWN  && selected + cols < item_count) selected += cols;
    if (down & KEY_UP) {
        if (selected - cols >= 0) selected -= cols;
    }
    (void)rows;

    if (down & KEY_R && item_count == MAX_ITEMS_PER_PAGE) { // next page
        st->library_page++;
        load_page(st);
    }
    if (down & KEY_L && st->library_page > 0) { // prev page
        st->library_page--;
        load_page(st);
    }

    if (down & KEY_A) {
        strncpy(st->current_item_id, items[selected].id, sizeof(st->current_item_id) - 1);
        st->current_screen = SCREEN_DETAILS;
    }
}

void screen_library_draw(AppState *st, C2D_TextBuf textbuf) {
    C2D_TargetClear(NULL, C_BG);

    C2D_Text title;
    C2D_TextParse(&title, textbuf, st->current_library_name);
    C2D_TextOptimize(&title);
    C2D_DrawText(&title, C2D_WithColor, PAD_LG, PAD_LG, 0.0f, 0.7f, 0.7f, C_ACCENT_MAIN);

    if (item_count == 0) {
        C2D_Text empty;
        C2D_TextParse(&empty, textbuf, "Nothing here.");
        C2D_TextOptimize(&empty);
        C2D_DrawText(&empty, C2D_WithColor, PAD_LG, 60.0f, 0.0f, 0.5f, 0.5f, C_TEXT_MUTED);
        return;
    }

    float grid_top = 48.0f;
    for (int i = 0; i < item_count; i++) {
        int col = i % GRID_COLS;
        int row = i / GRID_COLS;
        float x = PAD_LG + col * (POSTER_W + GRID_GAP);
        float y = grid_top + row * (POSTER_H + 20.0f);
        if (y > TOP_H) break; // off top screen; paging (L/R) covers the rest

        ui_draw_card(x, y, POSTER_W, POSTER_H, i == selected);

        // NOTE: poster art is not decoded/displayed yet -- Jellyfin serves
        // JPEG, which needs its own decode step (e.g. via a small libjpeg
        // build) and a citro3d texture upload. Left as a follow-up; for now
        // each card shows the title so the grid is still usable.
        C2D_Text label;
        C2D_TextParse(&label, textbuf, items[i].name);
        C2D_TextOptimize(&label);
        C2D_DrawText(&label, C2D_WithColor, x + 4.0f, y + POSTER_H + 2.0f, 0.0f, 0.35f, 0.35f, C_TEXT_MUTED);
    }

    char pageinfo[32];
    snprintf(pageinfo, sizeof(pageinfo), "Page %d   L/R to page, B back", st->library_page + 1);
    C2D_Text pg;
    C2D_TextParse(&pg, textbuf, pageinfo);
    C2D_TextOptimize(&pg);
    C2D_DrawText(&pg, C2D_WithColor, PAD_LG, TOP_H - 16.0f, 0.0f, 0.4f, 0.4f, C_TEXT_MUTED);
}
