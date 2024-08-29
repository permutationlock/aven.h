#ifndef AVEN_PATH_H
#define AVEN_PATH_H

#include "../aven.h"
#include "arena.h"
#include "str.h"

#define AVEN_PATH_MAX_ARGS 32
#define AVEN_PATH_MAX_LEN 4096

#ifdef _WIN32
    #define AVEN_PATH_SEP '\\'
#else
    #define AVEN_PATH_SEP '/'
#endif

AVEN_FN AvenStr aven_path(AvenArena *arena, char *path_str, ...);
AVEN_FN AvenStr aven_path_rel_dir(AvenStr path, AvenArena *arena);
AVEN_FN AvenStr aven_path_fname(AvenStr path, AvenArena *arena);
AVEN_FN bool aven_path_is_abs(AvenStr path);
AVEN_FN AvenStr aven_path_rel_intersect(
    AvenStr path1,
    AvenStr path2,
    AvenArena *arena
);
AVEN_FN AvenStr aven_path_rel_diff(
    AvenStr path1,
    AvenStr path2,
    AvenArena *arena
);

typedef Result(AvenStr) AvenPathResult;

AVEN_FN AvenPathResult aven_path_exe(AvenArena *arena);

#ifdef AVEN_IMPLEMENTATION

#include <stdarg.h>

#if defined(_WIN32)
    uint32_t GetModuleFileNameA(void *mod, char *buffer, uint32_t buffer_len);
#elif defined(__linux__)
    #if !defined(_POSIX_C_SOURCE) or _POSIX_C_SOURCE < 200112L
        #error "readlink requires _POSIX_C_SOURCE >= 200112L"
    #endif
    #include <unistd.h>
#endif

AVEN_FN AvenStr aven_path(AvenArena *arena, char *path_str, ...) {
    AvenStr path_data[AVEN_PATH_MAX_ARGS];
    AvenStrSlice path = { .len = 0, .ptr = path_data };

    path_data[0] = aven_str_cstr(path_str);
    path.len += 1;

    va_list args;
    va_start(args, path_str);
    for (
        char *cstr = va_arg(args, char *);
        cstr != NULL;
        cstr = va_arg(args, char *)
    ) {
        path_data[path.len] = aven_str_cstr(cstr);
        path.len += 1;
    }
    va_end(args);

    return aven_str_join(
        path,
        AVEN_PATH_SEP,
        arena
    );
}

AVEN_FN AvenStr aven_path_rel_dir(AvenStr path, AvenArena *arena) {
    size_t i;
    for (i = path.len; i > 0; i -= 1) {
        if (slice_get(path, i - 1) == AVEN_PATH_SEP) {
            break;
        }
    }
    if (i == 0) {
        return aven_str(".");
    }
    if (i == path.len) {
        return path;
    }
    AvenStr dir = { .len = i - 1 };
    dir.ptr = aven_arena_alloc(arena, dir.len + 1, 1),

    path.len = i - 1;
    slice_copy(dir, path);
    dir.ptr[dir.len] = 0;

    return dir;
}

AVEN_FN bool aven_path_is_abs(AvenStr path) {
#ifdef _WIN32
    if (slice_get(path, 1) == ':') {
        return true;
    }
    
    return slice_get(path, 0) == AVEN_PATH_SEP and
        slice_get(path, 1) == AVEN_PATH_SEP;
#else
    return slice_get(path, 0) == AVEN_PATH_SEP;
#endif
}

AVEN_FN AvenStr aven_path_rel_intersect(
    AvenStr path1,
    AvenStr path2,
    AvenArena *arena
) {
    if (slice_get(path1, 0) != slice_get(path2, 0)) {
        return aven_str("");
    }

    AvenArena temp_arena = *arena;

    AvenStrSlice path1_parts = aven_str_split(
        path1,
        AVEN_PATH_SEP,
        &temp_arena
    );
    AvenStrSlice path2_parts = aven_str_split(
        path2,
        AVEN_PATH_SEP,
        &temp_arena
    );

    size_t len = min(path1_parts.len, path2_parts.len);
    size_t same_index = 0;
    for (; same_index < len; same_index += 1) {
        bool match = aven_str_compare(
            slice_get(path1_parts, same_index),
            slice_get(path2_parts, same_index)
        );
        if (!match) {
            break;
        }
    }

    if (same_index == 0) {
        return aven_str("");
    }

    path1_parts.len = same_index;
    AvenStr intersect = aven_str_join(path1_parts, AVEN_PATH_SEP, &temp_arena);
    
    *arena = temp_arena;

    return intersect;
}

AVEN_FN AvenStr aven_path_rel_diff(
    AvenStr path1,
    AvenStr path2,
    AvenArena *arena
) {
    if (slice_get(path1, 0) != slice_get(path2, 0)) {
        return path1;
    }

    AvenArena temp_arena = *arena;

    AvenStrSlice path1_parts = aven_str_split(
        path1,
        AVEN_PATH_SEP,
        &temp_arena
    );
    AvenStrSlice path2_parts = aven_str_split(
        path2,
        AVEN_PATH_SEP,
        &temp_arena
    );

    size_t len = min(path1_parts.len, path2_parts.len);
    size_t same_index = 0;
    for (; same_index < len; same_index += 1) {
        bool match = aven_str_compare(
            slice_get(path1_parts, same_index),
            slice_get(path2_parts, same_index)
        );
        if (!match) {
            break;
        }
    }

    if (same_index == 0) {
        return path1;
    }

    AvenStrSlice diff_parts = {
        .len = path1_parts.len + path2_parts.len - 2 * same_index
    };
    diff_parts.ptr = aven_arena_create_array(AvenStr, arena, diff_parts.len);

    size_t diff_index = 0;
    for (size_t i = i; i < path2_parts.len; i += 1) {
        slice_get(diff_parts, diff_index) = aven_str("..");
        diff_index += 1;
    }
    for (size_t i = same_index; i < path1_parts.len; i += 1) {
        slice_get(diff_parts, diff_index) = slice_get(path1_parts, i);
        diff_index += 1;
    }

    AvenStr diff = aven_str_join(diff_parts, AVEN_PATH_SEP, &temp_arena);
    
    *arena = temp_arena;

    return diff;
}

AVEN_FN AvenStr aven_path_fname(AvenStr path, AvenArena *arena) {
    size_t i;
    for (i = path.len; i > 0; i -= 1) {
        if (slice_get(path, i - 1) == AVEN_PATH_SEP) {
            break;
        }
    }
    if (i == 0) {
        return path;
    }
    if (i == path.len) {
        return aven_str("");
    }
    AvenStr fname = { .len = path.len - i };
    fname.ptr = aven_arena_alloc(arena, fname.len + 1, 1);

    path.ptr += i;
    path.len -= i;
    slice_copy(fname, path);
    slice_get(fname, fname.len - 1) = 0;

    return fname;
}

AVEN_FN AvenPathResult aven_path_exe(AvenArena *arena) {
#ifdef _WIN32
    char buffer[AVEN_PATH_MAX_LEN];
    uint32_t len = GetModuleFileNameA(NULL, buffer, countof(buffer));
    if (len <= 0 or len == countof(buffer)) {
        return (AvenPathResult){ .error = 1 };
    }
 
    AvenStr path = { .len = len + 1 };
    path.ptr = aven_arena_alloc(arena, path.len, 1);

    memcpy(path.ptr, buffer, path.len - 1);
    slice_get(path, path.len - 1) = 0;

    return (AvenPathResult){ .payload = path };
#elif defined(__linux__)
    char buffer[AVEN_PATH_MAX_LEN];
    ssize_t len = readlink("/proc/self/exe", buffer, countof(buffer));
    if (len <= 0 or len == countof(buffer)) {
        return (AvenPathResult){ .error = 1 };
    }

    AvenStr path = { .len = (size_t)len + 1 };
    path.ptr = aven_arena_alloc(arena, path.len, 1);

    memcpy(path.ptr, buffer, path.len - 1);
    slice_get(path, path.len - 1) = 0;

    return (AvenPathResult){ .payload = path };
#else
    assert(false);
    return (AvenStrResult){ .error = 1 };
#endif
}

#endif // AVEN_IMPLEMENTATION

#endif // AVEN_PATH_H
