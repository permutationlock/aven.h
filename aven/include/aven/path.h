#ifndef AVEN_PATH_H
#define AVEN_PATH_H

#include <stdarg.h>

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

char *aven_path(AvenArena *arena, char *path_str, ...);
char *aven_path_dir(char *path, AvenArena *arena);
char *aven_path_fname(char *path, AvenArena *arena);
char *aven_path_exe(AvenArena *arena);

#if defined(AVEN_PATH_IMPLEMENTATION) or defined(AVEN_IMPLEMENTATION)

#ifdef _WIN32
    uint32_t GetModuleFileNameA(void *mod, char *buffer, uint32_t buffer_len);
#else
    #include <unistd.h>
#endif

char *aven_path(AvenArena *arena, char *path_str, ...) {
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

char *aven_path_dir(char *path, AvenArena *arena) {
    size_t path_len = strlen(path);
    size_t i;
    for (i = path_len; i > 0; i -= 1) {
        if (path[i - 1] == AVEN_PATH_SEP) {
            break;
        }
    }
    if (i == 0) {
        return "";
    }
    if (i == path_len) {
        return path;
    }
    char *dir_path = aven_arena_alloc(arena, i, 1);
    if (dir_path == NULL) {
        return NULL;
    }
    memcpy(dir_path, path, i - 1);
    dir_path[i - 1] = 0;
    return dir_path;
}

char *aven_path_fname(char *path, AvenArena *arena) {
    size_t path_len = strlen(path);
    size_t i;
    for (i = path_len; i > 0; i -= 1) {
        if (path[i - 1] == AVEN_PATH_SEP) {
            break;
        }
    }
    if (i == 0) {
        return path;
    }
    if (i == path_len) {
        return "";
    }
    size_t fname_len = path_len - i + 1;
    char *fname = aven_arena_alloc(arena, fname_len, 1);
    if (fname == NULL) {
        return NULL;
    }
    memcpy(fname, path + i, fname_len);
    fname[fname_len - 1] = 0;
    return fname;
}

char *aven_path_exe(AvenArena *arena) {
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

#endif // AVEN_PATH_IMPLEMENTATION

#endif // AVEN_PATH_H
