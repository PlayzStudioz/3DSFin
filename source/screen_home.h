#pragma once
#include "app_state.h"
#include <citro2d.h>

void screen_home_init(AppState *st);
void screen_home_update(AppState *st);
void screen_home_draw(AppState *st, C2D_TextBuf textbuf);
