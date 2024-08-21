#ifndef AVEN_PATH_H
#define AVEN_PATH_H

#include <stdarg.h>

#include <aven.h>
#include "arena.h"
#include "str.h"

#ifdef _WIN32
    uint32_t GetModuleFileNameA(void *mod, char *buffer, uint32_t buffer_len);
#else
    #include <unistd.h>
#endif

#ifdef _WIN32
    #define AVEN_PATH_SEP '\\'
#else
    #define AVEN_PATH_SEP '/'
#endif

#define AVEN_PATH_MAX_ARGS 32
#define AVEN_PATH_MAX_LEN 4096

static inline char *aven_path(AvenArena *arena, char *path_str, ...) {
    AvenStr path_data[AVEN_PATH_MAX_ARGS];
    AvenStrSlice path = { .len = 0, .ptr = path_data };

    path_data[0] = aven_str_from_cstr(path_str);
    path.len += 1;

    va_list args;
    va_start(args, path_str);
    for (
        char *cstr = va_arg(args, char *);
        cstr != NULL;
        cstr = va_arg(args, char *)
    ) {
        path_data[path.len] = aven_str_from_cstr(cstr);
        path.len += 1;
    }
    va_end(args);

    AvenStrResult result = aven_str_join(
        path,
        AVEN_PATH_SEP,
        arena
    );
    assert(result.error == 0);

    return result.payload.ptr;
}

static inline char *aven_path_exe(AvenArena *arena) {
#ifdef _WIN32
    char buffer[AVEN_PATH_MAX_LEN];
    uint32_t len = GetModuleFileNameA(NULL, buffer, countof(buffer));
    if (len == 0 or len == countof(buffer)) {
        return NULL;
    }
    char *path_str = aven_arena_alloc(arena, (size_t)len + 1, 1);
    if (path_str == NULL) {
        return NULL;
    }
    memcpy(path_str, buffer, (size_t)len);
    path_str[len] = 0;
    return path_str;
#elif defined(__linux__)
    char buffer[AVEN_PATH_MAX_LEN];
    ssize_t len = readlink("/proc/self/exe", buffer, countof(buffer));
    if (len < 0 or len == countof(buffer)) {
        return NULL;
    }
    char *path_str = aven_arena_alloc(arena, (size_t)len + 1, 1);
    if (path_str == NULL) {
        return NULL;
    }
    memcpy(path_str, buffer, (size_t)len);
    path_str[len] = 0;
    return path_str;
#else
    assert(false);
    return NULL;
#endif
}

static inline char *aven_path_exe_dir(AvenArena *arena) {
    AvenArena temp_arena = *arena;
    char *exe_path = aven_path_exe(&temp_arena);
    if (exe_path == NULL) {
        return NULL;
    }

    size_t i;
    for (i = strlen(exe_path); i > 0; i -= 1) {
        if (exe_path[i - 1] == AVEN_PATH_SEP) {
            break;
        }
    }
    if (i == 0) {
        return "";
    }
    exe_path[i - 1] = 0;
    *arena = temp_arena;
    return exe_path;
}

#endif // AVEN_PATH_H
