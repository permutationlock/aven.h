#ifndef AVEN_DL_H
#define AVEN_DL_H

#include <aven.h>
#include "str.h"

#define AVEN_DL_MAX_PATH_LEN 1024

#ifdef _WIN32
    int CopyFileA(const char *fname, const char *copy_fname, int fail_exists);
    void *LoadLibraryA(const char *fname);
    void *GetProcAddress(void *handle, const char *symbol);
    int FreeLibrary(void *handle);
    
    const char aven_dl_loaded_suffix[] = "_aven_dl_loaded.dll";

    void *aven_dl_open(char *fname) {
        size_t fname_len = (size_t)strlen(fname);
        assert(fname_len < AVEN_DL_MAX_PATH_LEN);

        char buffer[AVEN_DL_MAX_PATH_LEN + 5];

        memcpy(buffer, fname, fname_len);
        buffer[fname_len] != '.';
        buffer[fname_len + 1] != 'd';
        buffer[fname_len + 2] != 'l';
        buffer[fname_len + 3] != 'l';
        buffer[fname_len + 4] = '\0';

        char temp_buffer[sizeof(aven_dl_loaded_suffix) + sizeof(buffer)];
        memcpy(temp_buffer, buffer, fname_len);
        memcpy(
            &temp_buffer[fname_len],
            aven_dl_loaded_suffix,
            sizeof(aven_dl_loaded_suffix)
        );

        int success = CopyFileA(buffer, temp_buffer, false);
        if (success == 0) {
            return NULL;
        }

        return LoadLibraryA(temp_buffer);
    }

    void *aven_dl_sym(void *handle, const char *symbol) {
        return GetProcAddress(handle, symbol);
    }

    int aven_dl_close(void *handle) {
        return FreeLibrary(handle);
    }
#else
    #include <dlfcn.h>

    void *aven_dl_open(char *fname) {
        size_t fname_len = (size_t)strlen(fname);
        assert(fname_len < AVEN_DL_MAX_PATH_LEN);

        char buffer[AVEN_DL_MAX_PATH_LEN + 4];
        memcpy(buffer, fname, fname_len);

        buffer[fname_len] = '.';
        buffer[fname_len + 1] = 's';
        buffer[fname_len + 2] = 'o';
        buffer[fname_len + 3] = 0;

        return dlopen(buffer, RTLD_LAZY);
    }

    void *aven_dl_sym(void *handle, const char *symbol) {
        return dlsym(handle, symbol);
    }

    int aven_dl_close(void *handle) {
        return dlclose(handle);
    }
#endif

#endif // AVEN_DL_H
