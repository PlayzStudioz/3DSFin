#pragma once
#include "app_state.h"
#include <citro2d.h>

void screen_player_init(AppState *st);
void screen_player_update(AppState *st);
void screen_player_draw(AppState *st, C2D_TextBuf textbuf);
