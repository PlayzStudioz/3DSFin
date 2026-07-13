#include "screen_login.h"
#include "jf_api.h"
#include "ui_theme.h"
#include <3ds.h>
#include <string.h>
#include <stdio.h>

typedef enum { FIELD_SERVER = 0, FIELD_USER, FIELD_PASS, FIELD_CONNECT, FIELD_COUNT } Field;

static Field focused = FIELD_SERVER;
static char username[MAX_NAME_LEN] = "";
static char password[MAX_NAME_LEN] = "";
static char status_msg[128] = "Enter your Jellyfin server address to begin.";
static bool status_is_error = false;
static bool connecting = false;

void screen_login_init(AppState *st) {
    focused = FIELD_SERVER;
    if (!st->server_url[0]) strcpy(st->server_url, "http://");
    username[0] = 0;
    password[0] = 0;
    strcpy(status_msg, "Enter your Jellyfin server address to begin.");
    status_is_error = false;
}

// 3DS has no physical keyboard -- every text field opens the system
// software keyboard (swkbd) via ctrulib.
static void open_keyboard(const char *hint, char *buf, int buf_size, bool is_password) {
    SwkbdState swkbd;
    swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 2, -1);
    swkbdSetHintText(&swkbd, hint);
    swkbdSetInitialText(&swkbd, buf);
    if (is_password) swkbdSetPasswordMode(&swkbd, SWKBD_PASSWORD_HIDE_DELAY);
    SwkbdButton pressed = swkbdInputText(&swkbd, buf, buf_size);
    if (pressed == SWKBD_BUTTON_NONE) {
        // user backed out; leave buf untouched (swkbd may have partially
        // written -- restore is skipped here for simplicity, acceptable for v0.1)
    }
}

void screen_login_update(AppState *st) {
    hidScanInput();
    u32 down = hidKeysDown();

    if (down & KEY_DOWN)  focused = (Field)((focused + 1) % FIELD_COUNT);
    if (down & KEY_UP)    focused = (Field)((focused + FIELD_COUNT - 1) % FIELD_COUNT);

    if (down & KEY_A) {
        switch (focused) {
            case FIELD_SERVER:
                open_keyboard("http://192.168.1.x:8096", st->server_url, sizeof(st->server_url), false);
                break;
            case FIELD_USER:
                open_keyboard("Username", username, sizeof(username), false);
                break;
            case FIELD_PASS:
                open_keyboard("Password", password, sizeof(password), true);
                break;
            case FIELD_CONNECT: {
                connecting = true;
                strcpy(status_msg, "Connecting...");
                status_is_error = false;
                if (!jf_api_test_connection(st->server_url)) {
                    strcpy(status_msg, "Couldn't reach that server. Check the address.");
                    status_is_error = true;
                } else if (!jf_api_login(st, st->server_url, username, password)) {
                    strcpy(status_msg, "Login failed. Check username/password.");
                    status_is_error = true;
                } else {
                    st->current_screen = SCREEN_HOME;
                }
                connecting = false;
                break;
            }
            default: break;
        }
    }
}

static void draw_field(float y, const char *label, const char *value, bool focused_field,
                        bool mask, C2D_TextBuf textbuf) {
    ui_draw_card(PAD_LG, y, BOT_W - PAD_LG * 2, 36.0f, focused_field);

    C2D_Text t;
    char buf[192];
    if (mask && value[0]) {
        // render a fixed run of bullets instead of the real password
        int n = strlen(value);
        if (n > 20) n = 20;
        for (int i = 0; i < n; i++) buf[i] = '*';
        buf[n] = 0;
    } else {
        snprintf(buf, sizeof(buf), "%s", value[0] ? value : label);
    }
    C2D_TextParse(&t, textbuf, buf);
    C2D_TextOptimize(&t);
    C2D_DrawText(&t, C2D_WithColor, PAD_LG + PAD_MD, y + 10.0f, 0.0f, 0.5f, 0.5f,
                 value[0] ? C_TEXT_PRIMARY : C_TEXT_MUTED);
}

void screen_login_draw(AppState *st, C2D_TextBuf textbuf) {
    C2D_TargetClear(NULL, C_BG);

    C2D_Text title;
    C2D_TextParse(&title, textbuf, "3DSFin");
    C2D_TextOptimize(&title);
    C2D_DrawText(&title, C2D_WithColor, PAD_LG, PAD_LG, 0.0f, 0.9f, 0.9f, C_ACCENT_MAIN);

    C2D_Text sub;
    C2D_TextParse(&sub, textbuf, status_msg);
    C2D_TextOptimize(&sub);
    C2D_DrawText(&sub, C2D_WithColor, PAD_LG, 44.0f, 0.0f, 0.45f, 0.45f,
                 status_is_error ? C_ERROR : C_TEXT_MUTED);

    float y = 72.0f;
    draw_field(y, "Server address", st->server_url, focused == FIELD_SERVER, false, textbuf);
    y += 44.0f;
    draw_field(y, "Username", username, focused == FIELD_USER, false, textbuf);
    y += 44.0f;
    draw_field(y, "Password", password, focused == FIELD_PASS, true, textbuf);
    y += 44.0f;

    ui_draw_card(PAD_LG, y, BOT_W - PAD_LG * 2, 36.0f, focused == FIELD_CONNECT);
    C2D_Text btn;
    C2D_TextParse(&btn, textbuf, connecting ? "Connecting..." : "Connect  (A)");
    C2D_TextOptimize(&btn);
    C2D_DrawText(&btn, C2D_WithColor, PAD_LG + PAD_MD, y + 10.0f, 0.0f, 0.5f, 0.5f, C_TEXT_PRIMARY);
}
