#pragma once
#include <citro2d.h>

// Moonfin-inspired palette: deep near-black background, indigo/violet accent,
// warm off-white text, soft card surfaces. citro2d colors are 0xAABBGGRR.

#define C_BG           C2D_Color32(0x12,0x12,0x16,0xFF)  // near-black, faint blue tint
#define C_SURFACE      C2D_Color32(0x1E,0x1E,0x26,0xFF)  // card background
#define C_SURFACE_HI   C2D_Color32(0x2A,0x2A,0x36,0xFF)  // focused/hovered card
#define C_ACCENT_MAIN  C2D_Color32(0x8B,0x5C,0xF6,0xFF)  // violet accent (buttons, focus ring)
#define C_ACCENT_DIM   C2D_Color32(0x5A,0x40,0xA8,0xFF)
#define C_TEXT_PRIMARY C2D_Color32(0xF2,0xF0,0xEC,0xFF)  // warm off-white
#define C_TEXT_MUTED   C2D_Color32(0x9A,0x98,0xA4,0xFF)
#define C_ERROR        C2D_Color32(0xE5,0x5C,0x5C,0xFF)
#define C_SUCCESS      C2D_Color32(0x5C,0xC8,0x8A,0xFF)

// spacing / layout
#define PAD_SM   4.0f
#define PAD_MD   8.0f
#define PAD_LG   16.0f
#define CARD_RADIUS 6.0f

#define TOP_W  400.0f
#define TOP_H  240.0f
#define BOT_W  320.0f
#define BOT_H  240.0f

// poster grid on the library screen
#define POSTER_W   80.0f
#define POSTER_H   120.0f
#define GRID_COLS  4
#define GRID_GAP   8.0f

static inline void ui_draw_card(float x, float y, float w, float h, bool focused) {
    C2D_DrawRectSolid(x, y, 0.0f, w, h, focused ? C_SURFACE_HI : C_SURFACE);
    if (focused) {
        // thin accent border to indicate focus, since there's no mouse cursor --
        // navigation is entirely D-pad/Circle Pad driven.
        C2D_DrawRectSolid(x, y, 0.0f, w, 2.0f, C_ACCENT_MAIN);
        C2D_DrawRectSolid(x, y + h - 2.0f, 0.0f, w, 2.0f, C_ACCENT_MAIN);
        C2D_DrawRectSolid(x, y, 0.0f, 2.0f, h, C_ACCENT_MAIN);
        C2D_DrawRectSolid(x + w - 2.0f, y, 0.0f, 2.0f, h, C_ACCENT_MAIN);
    }
}
