#pragma once
#include "app_state.h"
#include <citro2d.h>

void screen_login_init(AppState *st);
void screen_login_update(AppState *st);
void screen_login_draw(AppState *st, C2D_TextBuf textbuf);
