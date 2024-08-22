#ifndef AVEN_STR_H
#define AVEN_STR_H

#include "../aven.h"
#include "arena.h"

typedef Slice(char) AvenStr;
typedef Result(AvenStr) AvenStrResult;
typedef Slice(AvenStr) AvenStrSlice;
typedef Result(AvenStrSlice) AvenStrSliceResult;

typedef enum {
    AVEN_STR_ERROR_NONE = 0,
    AVEN_STR_ERROR_ALLOC,
} AvenStrError;

#define aven_str(a) (AvenStr){ \
        .ptr = a, \
        .len = sizeof(a) - 1 \
    }

static inline AvenStr aven_str_from_cstr(char *cstr) {
    size_t len = 0;
    for (char *c = cstr; *c != 0; c += 1) {
        len += 1;
    }
    return (AvenStr){ .ptr = cstr, .len = len };
}

static inline bool aven_str_compare(AvenStr s1, AvenStr s2) {
    if (s1.len != s2.len) {
        return false;
    }
    for (size_t i = 0; i < s1.len; i += 1) {
        if (slice_get(s1, i) != slice_get(s2, i)) {
            return false;
        }
    }
    return true;
}

static inline AvenStrSliceResult aven_str_split(
    AvenStr str,
    char separator,
    AvenArena *arena
) {
    size_t nsep = 0;
    size_t after_last_sep = 0;
    for (size_t i = 0; i <= str.len; i += 1) {
        if (i == str.len or slice_get(str, i) == separator) {
            if (i - after_last_sep > 0) {
                nsep += 1;
            }
            after_last_sep = i + 1;
        }
    }

    AvenStr *split_mem = aven_arena_create_array(
        AvenStr,
        arena,
        nsep
    );
    if (split_mem == NULL) {
        return (AvenStrSliceResult){
            .error = AVEN_STR_ERROR_ALLOC,
        };
    }

    AvenStrSlice split_strs = {
        .ptr = split_mem,
        .len = nsep,
    };

    size_t string_index = 0;
    after_last_sep = 0;
    for (size_t i = 0; i <= str.len; i += 1) {
        if (i == str.len or slice_get(str, i) == separator) {
            size_t len = i - after_last_sep;
            if (len > 0) {
                char *string_mem = aven_arena_alloc(arena, len + 1, 1);
                if (string_mem == NULL) {
                    return (AvenStrSliceResult){
                        .error = AVEN_STR_ERROR_ALLOC,
                    };
                }

                slice_get(split_strs, string_index) = (AvenStr){
                    .ptr = string_mem,
                    .len = len,
                };
                memcpy(string_mem, str.ptr + after_last_sep, len);
                string_mem[len] = 0;

                string_index += 1;
            }

            after_last_sep = i + 1;
        }
    }

    return (AvenStrSliceResult){ .payload = split_strs };
}

static inline AvenStrResult aven_str_concat(
    AvenStr s1,
    AvenStr s2,
    AvenArena *arena
) {
    char *str_mem = aven_arena_alloc(arena, s1.len + s2.len + 1, 1);
    if (str_mem == NULL) {
        return (AvenStrResult){ .error = AVEN_STR_ERROR_ALLOC };
    }

    AvenStr new_string = { .ptr = str_mem, .len = s1.len + s2.len };
    slice_copy(new_string, s1);

    AvenStr rest_string = {
        .ptr = new_string.ptr + s1.len,
        .len = new_string.len - s1.len,
    };
    slice_copy(rest_string, s2);

    new_string.ptr[new_string.len] = 0;

    return (AvenStrResult){ .payload = new_string };
}

static inline AvenStrResult aven_str_join(
    AvenStrSlice strings,
    char separator,
    AvenArena *arena
) {
    size_t len = 0;
    for (size_t i = 0; i < strings.len; i += 1) {
        AvenStr cur_str = slice_get(strings, i);
        if (cur_str.len == 0) {
            continue;
        }

        len += slice_get(strings, i).len;
        if ((i + 1) < strings.len) {
            len += 1;
        }
    }
   
    char *str_mem = aven_arena_alloc(arena, len + 1, 1);
    if (str_mem == NULL) {
        return (AvenStrResult){ .error = AVEN_STR_ERROR_ALLOC };
    }

    AvenStr new_str = { .ptr = str_mem, .len = len };
    AvenStr rest_str = new_str;

    for (size_t i = 0; i < strings.len; i += 1) {
        AvenStr cur_str = slice_get(strings, i);
        if (cur_str.len == 0) {
            continue;
        }

        slice_copy(rest_str, cur_str);

        rest_str.ptr += cur_str.len;
        rest_str.len -= cur_str.len;

        if ((i + 1) < strings.len) {
            slice_get(rest_str, 0) = separator;
            rest_str.ptr += 1;
            rest_str.len -= 1;
        }
    }

    new_str.ptr[new_str.len] = 0;

    return (AvenStrResult){ .payload = new_str };
}

#endif // AVEN_STR_H
