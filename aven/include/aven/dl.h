#ifndef AVEN_DL_H
#define AVEN_DL_H

#include "../aven.h"
#include "str.h"

#define AVEN_DL_MAX_PATH_LEN 4096

void *aven_dl_open(AvenStr fname);
void *aven_dl_sym(void *handle, AvenStr symbol);
int aven_dl_close(void *handle);

#if defined(AVEN_DL_IMPLEMENTATION) or defined(AVEN_IMPLEMENTATION)

#ifdef _WIN32
    #include "path.h"

    int CopyFileA(const char *fname, const char *copy_fname, int fail_exists);
    void *LoadLibraryA(const char *fname);
    void *GetProcAddress(void *handle, const char *symbol);
    int FreeLibrary(void *handle);
    uint32_t GetTempPathA(uint32_t buffer_len, char *buffer);
    
    const char aven_dl_suffix[] = "_aven_dl_loaded.dll";

    void *aven_dl_open(AvenStr fname) {
        assert(fname.len > 0);
        assert(fname.len < AVEN_DL_MAX_PATH_LEN);

        char buffer[AVEN_DL_MAX_PATH_LEN + 5];

        memcpy(buffer, fname.ptr, fname.len);
        buffer[fname.len] = '.';
        buffer[fname.len + 1] = 'd';
        buffer[fname.len + 2] = 'l';
        buffer[fname.len + 3] = 'l';
        buffer[fname.len + 4] = '\0';

        char temp_buffer[
            AVEN_DL_MAX_PATH_LEN +
            sizeof(aven_dl_suffix) +
            sizeof(buffer)
        ];
        uint32_t temp_path_len = GetTempPathA(AVEN_DL_MAX_PATH_LEN, temp_buffer);
        if (temp_path_len == 0) {
            // If we can't create a temp file, just make one in this directory
            memcpy(temp_buffer, buffer, fname.len);
            memcpy(
                &temp_buffer[fname.len],
                aven_dl_suffix,
                sizeof(aven_dl_suffix)
            );
        } else {
            if (temp_buffer[temp_path_len - 1] != AVEN_PATH_SEP) {
                temp_buffer[temp_path_len] = AVEN_PATH_SEP;
                temp_path_len += 1;
            }
            char arena_buffer[AVEN_DL_MAX_PATH_LEN];
            AvenArena carena = aven_arena_init(
                arena_buffer,
                sizeof(arena_buffer)
            );
            AvenStr libname = aven_path_fname(fname, &carena);
            memcpy(&temp_buffer[temp_path_len], libname.ptr, libname.len);
            temp_path_len += libname.len;
            memcpy(
                &temp_buffer[temp_path_len],
                aven_dl_suffix,
                sizeof(aven_dl_suffix)
            );
        }

        int success = CopyFileA(buffer, temp_buffer, false);
        if (success == 0) {
            return NULL;
        }

        return LoadLibraryA(temp_buffer);
    }

    void *aven_dl_sym(void *handle, AvenStr symbol) {
        assert(symbol.len > 0);
        return GetProcAddress(handle, symbol.ptr);
    }

    int aven_dl_close(void *handle) {
        return FreeLibrary(handle);
    }
#else
    #include <dlfcn.h>

    void *aven_dl_open(AvenStr fname) {
        assert(fname.len > 0);
        assert(fname.len < AVEN_DL_MAX_PATH_LEN);

        char buffer[AVEN_DL_MAX_PATH_LEN + 4];
        memcpy(buffer, fname.ptr, fname.len);

        buffer[fname.len] = '.';
        buffer[fname.len + 1] = 's';
        buffer[fname.len + 2] = 'o';
        buffer[fname.len + 3] = 0;

        return dlopen(buffer, RTLD_LAZY);
    }

    void *aven_dl_sym(void *handle, AvenStr symbol) {
        assert(symbol.len > 0);
        return dlsym(handle, symbol.ptr);
    }

    int aven_dl_close(void *handle) {
        return dlclose(handle);
    }
#endif

#endif // AVEN_DL_IMPLEMENTATION

#endif // AVEN_DL_H
