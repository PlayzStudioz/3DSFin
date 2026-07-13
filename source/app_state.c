#include "app_state.h"
#include <string.h>

void app_state_init(AppState *st) {
    memset(st, 0, sizeof(AppState));
    st->quality = QUALITY_LOW;
    st->current_screen = SCREEN_LOGIN;
    st->previous_screen = SCREEN_LOGIN;
    st->library_page = 0;
    st->logged_in = false;
}
