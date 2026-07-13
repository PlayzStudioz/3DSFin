#pragma once
#include "app_state.h"
#include <citro2d.h>

void screen_details_init(AppState *st);
void screen_details_update(AppState *st);
void screen_details_draw(AppState *st, C2D_TextBuf textbuf);
