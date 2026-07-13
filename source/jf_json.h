#pragma once
// A deliberately small, read-only JSON parser -- just enough to pull fields
// out of Jellyfin API responses without pulling in a full third-party JSON
// library and its build-system integration. Not a general-purpose parser:
// no writer, no in-place mutation, values are read via path lookups.

typedef enum {
    JF_NULL, JF_BOOL, JF_NUMBER, JF_STRING, JF_ARRAY, JF_OBJECT
} JfType;

typedef struct JfValue JfValue;
struct JfValue {
    JfType type;
    union {
        bool boolean;
        double number;
        struct { const char *ptr; int len; } string; // points into original buffer
    };
    // object: linked list of key/value; array: linked list of value (key ignored)
    struct JfMember *first_child;
    JfValue *next; // sibling, used when iterating arrays
};

typedef struct JfMember {
    const char *key; int key_len;
    JfValue *value;
    struct JfMember *next;
} JfMember;

typedef struct {
    const char *buf;
    int len, pos;
    // simple bump allocator; caller provides the arena so we don't malloc per-node
    char *arena; int arena_size, arena_used;
} JfParser;

// Parse `json_text` (does not need to be null-terminated if len is given).
// `arena`/`arena_size` back all allocations made during parsing; pick e.g.
// 64KB for a typical item-detail response. Returns NULL on parse failure.
JfValue *jf_parse(const char *json_text, int len, char *arena, int arena_size);

// Convenience accessors. All return NULL/0/false if the path doesn't exist
// or is the wrong type -- callers should always check before use.
JfValue *jf_get(JfValue *obj, const char *key);          // object member lookup
JfValue *jf_index(JfValue *arr, int i);                    // array element lookup
int      jf_array_len(JfValue *arr);
// Copies into `out` (size `out_size`), null-terminated, truncating if needed.
void     jf_get_string(JfValue *obj, const char *key, char *out, int out_size);
double   jf_get_number(JfValue *obj, const char *key, double fallback);
bool     jf_get_bool(JfValue *obj, const char *key, bool fallback);
