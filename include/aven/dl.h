#ifndef AVEN_DL_H
#define AVEN_DL_H

#ifdef _WIN32
    int CopyFileA(const char *fname, const char *copy_fname, int fail_exists);
    void *LoadLibraryA(const char *fname);
    void *GetProcAddress(void *handle, const char *symbol);
    int FreeLibrary(void *handle);
    
    const char aven_dl_loaded_suffix[] = "_aven_dl_loaded.dll";

    void *aven_dl_open(const char *fname) {
        char buffer[1024];
        size_t i;
        for (i = 0; fname[i] != '\0' and i < sizeof(buffer) - 1; ++i) {
            if (fname[i] == '/') {
                buffer[i] = '\\';
            } else {
                buffer[i] = fname[i];
            }
        }
        buffer[i] = '\0';

        if (
            i < 5 or
            buffer[i - 4] != '.' or
            buffer[i - 3] != 'd' or
            buffer[i - 2] != 'l' or
            buffer[i - 1] != 'l'
        ) {
            return NULL;
        }

        char temp_buffer[sizeof(aven_dl_loaded_suffix) + sizeof(buffer)];
        memcpy(temp_buffer, buffer, i - 4);
        memcpy(
            &temp_buffer[i - 4],
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

    void *aven_dl_open(const char *fname) {
        return dlopen(fname, RTLD_LAZY);
    }

    void *aven_dl_sym(void *handle, const char *symbol) {
        return dlsym(handle, symbol);
    }

    int aven_dl_close(void *handle) {
        return dlclose(handle);
    }
#endif

#endif // AVEN_DL_H
