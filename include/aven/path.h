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

AvenStr aven_path(AvenArena *arena, char *path_str, ...);
AvenStr aven_path_dir(AvenStr path, AvenArena *arena);
AvenStr aven_path_fname(AvenStr path, AvenArena *arena);

typedef Result(AvenStr) AvenPathResult;

AvenPathResult aven_path_exe(AvenArena *arena);

#if defined(AVEN_PATH_IMPLEMENTATION) or defined(AVEN_IMPLEMENTATION)

#include <stdarg.h>

#if defined(_WIN32)
    uint32_t GetModuleFileNameA(void *mod, char *buffer, uint32_t buffer_len);
#elif defined(__linux__)
    #if !defined(_POSIX_C_SOURCE) or _POSIX_C_SOURCE < 200112L
        #error "readlink requires _POSIX_C_SOURCE >= 200112L"
    #endif
    #include <unistd.h>
#endif

AvenStr aven_path(AvenArena *arena, char *path_str, ...) {
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

AvenStr aven_path_dir(AvenStr path, AvenArena *arena) {
    size_t i;
    for (i = path.len; i > 0; i -= 1) {
        if (slice_get(path, i - 1) == AVEN_PATH_SEP) {
            break;
        }
    }
    if (i == 0) {
        return aven_str("");
    }
    if (i == path.len) {
        return path;
    }
    AvenStr dir = { .len = i };
    dir.ptr =  aven_arena_alloc(arena, dir.len, 1),

    path.len = i - 1;
    slice_copy(dir, path);
    slice_get(dir, i - 1) = 0;

    return dir;
}

AvenStr aven_path_fname(AvenStr path, AvenArena *arena) {
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
    AvenStr fname = { .len = path.len - i + 1 };
    fname.ptr = aven_arena_alloc(arena, fname.len, 1);

    path.ptr += i;
    path.len -= i;
    slice_copy(fname, path);
    slice_get(fname, fname.len - 1) = 0;

    return fname;
}

AvenPathResult aven_path_exe(AvenArena *arena) {
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

#endif // AVEN_PATH_IMPLEMENTATION

#endif // AVEN_PATH_H
