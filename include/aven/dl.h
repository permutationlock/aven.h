#ifndef AVEN_DL_H
#define AVEN_DL_H

#include "../aven.h"
#include "str.h"

#define AVEN_DL_MAX_PATH_LEN 4096

AVEN_FN void *aven_dl_open(AvenStr fname);
AVEN_FN void *aven_dl_sym(void *handle, AvenStr symbol);
AVEN_FN int aven_dl_close(void *handle);

#ifdef AVEN_IMPLEMENTATION

#ifdef _WIN32
    AVEN_FN void *aven_dl_open(AvenStr fname) {
        AVEN_WIN32_FN(void *) LoadLibraryA(const char *fname);
        AVEN_WIN32_FN(int) CopyFileA(
            const char *fname,
            const char *copy_fname,
            int fail_exists
        );

        assert(fname.len < AVEN_DL_MAX_PATH_LEN);

        if (fname.len < 5) {
            return NULL;
        }

        int dot_index = (int)fname.len - 1;
        for (; dot_index > 0; dot_index -= 1) {
            if (slice_get(fname, (size_t)dot_index) == '.') {
                break;
            }
        }

        if (dot_index == 0) {
            return NULL;
        }

        char aven_dl_suffix[] = "_aven_dl_loaded.dll";
        char temp_buffer[
            AVEN_DL_MAX_PATH_LEN +
            sizeof(aven_dl_suffix)
        ];
        memcpy(temp_buffer, fname.ptr, (size_t)dot_index);
        memcpy(
            &temp_buffer[dot_index],
            aven_dl_suffix,
            sizeof(aven_dl_suffix)
        );

        int success = CopyFileA(fname.ptr, temp_buffer, false);
        if (success == 0) {
            return NULL;
        }

        return LoadLibraryA(temp_buffer);
    }

    AVEN_FN void *aven_dl_sym(void *handle, AvenStr symbol) {
        AVEN_WIN32_FN(void *) GetProcAddress(
            void *handle,
            const char *symbol
        );

        return GetProcAddress(handle, symbol.ptr);
    }

    AVEN_FN int aven_dl_close(void *handle) {
        AVEN_WIN32_FN(int) FreeLibrary(void *handle);

        return FreeLibrary(handle);
    }
#else
    #include <dlfcn.h>

    AVEN_FN void *aven_dl_open(AvenStr fname) {
        return dlopen(fname.ptr, RTLD_LAZY);
    }

    AVEN_FN void *aven_dl_sym(void *handle, AvenStr symbol) {
        return dlsym(handle, symbol.ptr);
    }

    AVEN_FN int aven_dl_close(void *handle) {
        return dlclose(handle);
    }
#endif

#endif // AVEN_IMPLEMENTATION

#endif // AVEN_DL_H
