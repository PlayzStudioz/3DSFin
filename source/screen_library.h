#pragma once
#include "app_state.h"
#include <citro2d.h>

void screen_library_init(AppState *st);
void screen_library_update(AppState *st);
void screen_library_draw(AppState *st, C2D_TextBuf textbuf);
