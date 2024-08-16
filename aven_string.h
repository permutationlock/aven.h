#ifndef AVEN_STRING_H
#define AVEN_STRING_H

#include <string.h>

#include "aven.h"
#include "aven_arena.h"

typedef Slice(char) AvenString;
typedef Result(AvenString) AvenStringResult;
typedef Slice(AvenString) AvenStringSlice;
typedef Result(AvenStringSlice) AvenStringSliceResult;

typedef enum {
    AVEN_STRING_ERROR_NONE = 0,
    AVEN_STRING_ERROR_ALLOC,
} AvenStringError;

#define aven_string_from_literal(a) (AvenString){ \
        .ptr = a, \
        .len = sizeof(a) - 1 \
    }

static inline AvenString aven_string_from_cstr(char *cstr) {
    return (AvenString){ .ptr = cstr, .len = strlen(cstr) };
}

static inline AvenStringSliceResult aven_string_split(
    AvenString str,
    char separator,
    AvenArena *arena
) {
    size_t nsep = 0;
    size_t after_last_sep = 0;
    for (size_t i = 0; i < str.len; i += 1) {
        if (slice_get(str, i) == separator or (i + 1) == str.len) {
            if (i - after_last_sep > 0) {
                nsep += 1;
            }
            after_last_sep = i + 1;
        }
    }

    AvenString *split_mem = aven_arena_create_array(
        AvenString,
        arena,
        nsep
    );
    if (split_mem == NULL) {
        return (AvenStringSliceResult){
            .error = AVEN_STRING_ERROR_ALLOC,
        };
    }

    AvenStringSlice split_strs = {
        .ptr = split_mem,
        .len = nsep,
    };

    size_t string_index = 0;
    after_last_sep = 0;
    for (size_t i = 0; i < str.len; i += 1) {
        if (slice_get(str, i) == separator or (i + 1) == str.len) {
            size_t len = i - after_last_sep;
            if (len > 0) {
                char *string_mem = aven_arena_alloc(arena, len + 1, 1);
                if (string_mem == NULL) {
                    return (AvenStringSliceResult){
                        .error = AVEN_STRING_ERROR_ALLOC,
                    };
                }

                slice_get(split_strs, string_index) = (AvenString){
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

    return (AvenStringSliceResult){ .payload = split_strs };
}

static inline AvenStringResult aven_string_concat(
    AvenString s1,
    AvenString s2,
    AvenArena *arena
) {
    char *str_mem = aven_arena_alloc(arena, s1.len + s2.len + 1, 1);
    if (str_mem == NULL) {
        return (AvenStringResult){ .error = AVEN_STRING_ERROR_ALLOC };
    }

    AvenString new_string = { .ptr = str_mem, .len = s1.len + s2.len };
    slice_copy(new_string, s1);

    AvenString rest_string = {
        .ptr = new_string.ptr + s1.len,
        .len = new_string.len - s1.len,
    };
    slice_copy(rest_string, s2);

    new_string.ptr[new_string.len] = 0;

    return (AvenStringResult){ .payload = new_string };
}

static inline AvenStringResult aven_string_join(
    AvenStringSlice strings,
    char separator,
    AvenArena *arena
) {
    size_t len = 0;
    for (size_t i = 0; i < strings.len; i += 1) {
        AvenString cur_str = slice_get(strings, i);
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
        return (AvenStringResult){ .error = AVEN_STRING_ERROR_ALLOC };
    }

    AvenString new_str = { .ptr = str_mem, .len = len };
    AvenString rest_str = new_str;

    for (size_t i = 0; i < strings.len; i += 1) {
        AvenString cur_str = slice_get(strings, i);
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

    return (AvenStringResult){ .payload = new_str };
}

#endif // AVEN_STRING_H
