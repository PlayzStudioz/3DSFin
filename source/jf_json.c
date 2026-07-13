#include "jf_json.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

static void *arena_alloc(JfParser *p, int size) {
    // 4-byte align
    size = (size + 3) & ~3;
    if (p->arena_used + size > p->arena_size) return NULL;
    void *ptr = p->arena + p->arena_used;
    p->arena_used += size;
    return ptr;
}

static void skip_ws(JfParser *p) {
    while (p->pos < p->len) {
        char c = p->buf[p->pos];
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') p->pos++;
        else break;
    }
}

static JfValue *parse_value(JfParser *p);

static JfValue *new_value(JfParser *p, JfType t) {
    JfValue *v = (JfValue *)arena_alloc(p, sizeof(JfValue));
    if (!v) return NULL;
    memset(v, 0, sizeof(JfValue));
    v->type = t;
    return v;
}

static bool parse_string_raw(JfParser *p, const char **out_ptr, int *out_len) {
    if (p->buf[p->pos] != '"') return false;
    p->pos++;
    const char *start = p->buf + p->pos;
    // We don't unescape in place (read-only buffer) -- callers get the raw
    // slice including any backslash escapes. jf_get_string() unescapes the
    // common cases (\", \\, \n, \/) when copying out.
    while (p->pos < p->len && p->buf[p->pos] != '"') {
        if (p->buf[p->pos] == '\\' && p->pos + 1 < p->len) p->pos++;
        p->pos++;
    }
    if (p->pos >= p->len) return false;
    *out_ptr = start;
    *out_len = (int)(p->buf + p->pos - start);
    p->pos++; // closing quote
    return true;
}

static JfValue *parse_object(JfParser *p) {
    JfValue *v = new_value(p, JF_OBJECT);
    if (!v) return NULL;
    p->pos++; // '{'
    skip_ws(p);
    JfMember *tail = NULL;
    if (p->pos < p->len && p->buf[p->pos] == '}') { p->pos++; return v; }
    while (p->pos < p->len) {
        skip_ws(p);
        const char *kptr; int klen;
        if (!parse_string_raw(p, &kptr, &klen)) return NULL;
        skip_ws(p);
        if (p->pos >= p->len || p->buf[p->pos] != ':') return NULL;
        p->pos++;
        skip_ws(p);
        JfValue *val = parse_value(p);
        if (!val) return NULL;
        JfMember *m = (JfMember *)arena_alloc(p, sizeof(JfMember));
        if (!m) return NULL;
        m->key = kptr; m->key_len = klen; m->value = val; m->next = NULL;
        if (tail) tail->next = m; else v->first_child = m;
        tail = m;
        skip_ws(p);
        if (p->pos < p->len && p->buf[p->pos] == ',') { p->pos++; continue; }
        if (p->pos < p->len && p->buf[p->pos] == '}') { p->pos++; break; }
        return NULL;
    }
    return v;
}

static JfValue *parse_array(JfParser *p) {
    JfValue *v = new_value(p, JF_ARRAY);
    if (!v) return NULL;
    p->pos++; // '['
    skip_ws(p);
    JfValue *tail = NULL;
    if (p->pos < p->len && p->buf[p->pos] == ']') { p->pos++; return v; }
    while (p->pos < p->len) {
        skip_ws(p);
        JfValue *val = parse_value(p);
        if (!val) return NULL;
        if (tail) tail->next = val; else v->first_child = (JfMember *)val;
        // NOTE: array elements are chained via JfValue.next; we stash the
        // head in first_child cast to JfMember* purely as a storage slot.
        tail = val;
        skip_ws(p);
        if (p->pos < p->len && p->buf[p->pos] == ',') { p->pos++; continue; }
        if (p->pos < p->len && p->buf[p->pos] == ']') { p->pos++; break; }
        return NULL;
    }
    return v;
}

static JfValue *parse_value(JfParser *p) {
    skip_ws(p);
    if (p->pos >= p->len) return NULL;
    char c = p->buf[p->pos];
    if (c == '{') return parse_object(p);
    if (c == '[') return parse_array(p);
    if (c == '"') {
        const char *ptr; int len;
        if (!parse_string_raw(p, &ptr, &len)) return NULL;
        JfValue *v = new_value(p, JF_STRING);
        if (!v) return NULL;
        v->string.ptr = ptr; v->string.len = len;
        return v;
    }
    if (c == 't' && p->pos + 4 <= p->len && strncmp(p->buf + p->pos, "true", 4) == 0) {
        p->pos += 4; JfValue *v = new_value(p, JF_BOOL); if (v) v->boolean = true; return v;
    }
    if (c == 'f' && p->pos + 5 <= p->len && strncmp(p->buf + p->pos, "false", 5) == 0) {
        p->pos += 5; JfValue *v = new_value(p, JF_BOOL); if (v) v->boolean = false; return v;
    }
    if (c == 'n' && p->pos + 4 <= p->len && strncmp(p->buf + p->pos, "null", 4) == 0) {
        p->pos += 4; return new_value(p, JF_NULL);
    }
    if (c == '-' || isdigit((unsigned char)c)) {
        const char *start = p->buf + p->pos;
        int start_pos = p->pos;
        p->pos++;
        while (p->pos < p->len) {
            char d = p->buf[p->pos];
            if (isdigit((unsigned char)d) || d == '.' || d == 'e' || d == 'E' || d == '+' || d == '-') p->pos++;
            else break;
        }
        char tmp[64];
        int n = p->pos - start_pos;
        if (n >= (int)sizeof(tmp)) n = sizeof(tmp) - 1;
        memcpy(tmp, start, n); tmp[n] = 0;
        JfValue *v = new_value(p, JF_NUMBER);
        if (v) v->number = atof(tmp);
        return v;
    }
    return NULL;
}

JfValue *jf_parse(const char *json_text, int len, char *arena, int arena_size) {
    JfParser p = { .buf = json_text, .len = len, .pos = 0,
                   .arena = arena, .arena_size = arena_size, .arena_used = 0 };
    return parse_value(&p);
}

JfValue *jf_get(JfValue *obj, const char *key) {
    if (!obj || obj->type != JF_OBJECT) return NULL;
    int klen = (int)strlen(key);
    for (JfMember *m = obj->first_child; m; m = m->next) {
        if (m->key_len == klen && strncmp(m->key, key, klen) == 0) return m->value;
    }
    return NULL;
}

JfValue *jf_index(JfValue *arr, int i) {
    if (!arr || arr->type != JF_ARRAY) return NULL;
    JfValue *cur = (JfValue *)arr->first_child;
    while (cur && i > 0) { cur = cur->next; i--; }
    return (i == 0) ? cur : NULL;
}

int jf_array_len(JfValue *arr) {
    if (!arr || arr->type != JF_ARRAY) return 0;
    int n = 0;
    for (JfValue *cur = (JfValue *)arr->first_child; cur; cur = cur->next) n++;
    return n;
}

static void unescape_copy(const char *src, int src_len, char *out, int out_size) {
    int oi = 0;
    for (int i = 0; i < src_len && oi < out_size - 1; i++) {
        if (src[i] == '\\' && i + 1 < src_len) {
            i++;
            switch (src[i]) {
                case 'n': out[oi++] = '\n'; break;
                case 't': out[oi++] = '\t'; break;
                case '"': out[oi++] = '"'; break;
                case '\\': out[oi++] = '\\'; break;
                case '/': out[oi++] = '/'; break;
                default: out[oi++] = src[i]; break; // \uXXXX etc left as-is (rare in Jellyfin text fields)
            }
        } else {
            out[oi++] = src[i];
        }
    }
    out[oi] = 0;
}

void jf_get_string(JfValue *obj, const char *key, char *out, int out_size) {
    out[0] = 0;
    JfValue *v = jf_get(obj, key);
    if (!v || v->type != JF_STRING) return;
    unescape_copy(v->string.ptr, v->string.len, out, out_size);
}

double jf_get_number(JfValue *obj, const char *key, double fallback) {
    JfValue *v = jf_get(obj, key);
    if (!v || v->type != JF_NUMBER) return fallback;
    return v->number;
}

bool jf_get_bool(JfValue *obj, const char *key, bool fallback) {
    JfValue *v = jf_get(obj, key);
    if (!v || v->type != JF_BOOL) return fallback;
    return v->boolean;
}
