#ifndef AVEN_FS_H
#define AVEN_FS_H

#include "../aven.h"
#include "str.h"

typedef enum {
    AVEN_FS_RM_ERROR_NONE = 0,
    AVEN_FS_RM_ERROR_BADPATH,
    AVEN_FS_RM_ERROR_ACCESS,
    AVEN_FS_RM_ERROR_OTHER,
} AvenFsRmError;

AVEN_FN int aven_fs_rm(AvenStr path);

typedef enum {
    AVEN_FS_RMDIR_ERROR_NONE = 0,
    AVEN_FS_RMDIR_ERROR_NOTEMPTY,
    AVEN_FS_RMDIR_ERROR_BADPATH,
    AVEN_FS_RMDIR_ERROR_ACCESS,
    AVEN_FS_RMDIR_ERROR_OTHER,
} AvenFsRmdirError;

AVEN_FN int aven_fs_rmdir(AvenStr path);

typedef enum {
    AVEN_FS_MKDIR_ERROR_NONE = 0,
    AVEN_FS_MKDIR_ERROR_BADPATH,
    AVEN_FS_MKDIR_ERROR_ACCESS,
    AVEN_FS_MKDIR_ERROR_EXIST,
    AVEN_FS_MKDIR_ERROR_OTHER,
} AvenFsMkdirError;

AVEN_FN int aven_fs_mkdir(AvenStr path);

typedef enum {
    AVEN_FS_TRUNC_ERROR_NONE = 0,
    AVEN_FS_TRUNC_ERROR_BADPATH,
    AVEN_FS_TRUNC_ERROR_ACCESS,
    AVEN_FS_TRUNC_ERROR_OTHER,
} AvenFsTruncError;

AVEN_FN int aven_fs_trunc(AvenStr path);

typedef enum {
    AVEN_FS_COPY_ERROR_NONE = 0,
    AVEN_FS_COPY_ERROR_IFOPEN,
    AVEN_FS_COPY_ERROR_OFOPEN,
    AVEN_FS_COPY_ERROR_IFREAD,
    AVEN_FS_COPY_ERROR_OFWRITE,
    AVEN_FS_COPY_ERROR_OTHER,
} AvenFsCopyError;

AVEN_FN int aven_fs_copy(AvenStr ipath, AvenStr opath);

AVEN_FN void aven_fs_utf8_mode(void);

#ifdef AVEN_IMPLEMENTATION

#include <errno.h>

#ifdef _WIN32
    #if defined(_MSC_VER) and defined(__clang__)
        #pragma clang diagnostic push
        #pragma clang diagnostic ignored "-Wdeprecated-declarations"
    #endif
    #include <direct.h>
    #include <fcntl.h>
    #include <io.h>
    #include <sys/stat.h>
#else
    #include <fcntl.h>
    #include <sys/stat.h>
    #include <unistd.h>
#endif

AVEN_FN int aven_fs_rm(AvenStr path) {
#ifdef _WIN32
    int error = _unlink(path.ptr);
    if (error != 0) {
        switch (errno) {
            case EACCES:
                return AVEN_FS_RM_ERROR_ACCESS;
            case ENOENT:
                return AVEN_FS_RM_ERROR_BADPATH;
            default:
                return AVEN_FS_RM_ERROR_OTHER;
        }
    }

    return 0;
#else
    int error = unlink(path.ptr);
    if (error != 0) {
        switch (errno) {
            case EACCES:
                return AVEN_FS_RM_ERROR_ACCESS;
            case ENOENT:
                return AVEN_FS_RM_ERROR_BADPATH;
            case EBUSY:
                return AVEN_FS_RM_ERROR_ACCESS;
            case ENOTDIR:
            case EISDIR:
                return AVEN_FS_RM_ERROR_BADPATH;
            default:
                return AVEN_FS_RM_ERROR_OTHER;
        }
    }

    return 0;
#endif
}

AVEN_FN int aven_fs_rmdir(AvenStr path) {
#ifdef _WIN32
    int error = _rmdir(path.ptr);
    if (error != 0) {
        switch (errno) {
            case ENOTEMPTY:
                return AVEN_FS_RMDIR_ERROR_NOTEMPTY;
            case EACCES:
                return AVEN_FS_RMDIR_ERROR_ACCESS;
            case ENOENT:
                return AVEN_FS_RMDIR_ERROR_BADPATH;
            default:
                return AVEN_FS_RMDIR_ERROR_OTHER;
        }
    }

    return 0;
#else
    int error = rmdir(path.ptr);
    if (error != 0) {
        switch (errno) {
            case ENOTEMPTY:
                return AVEN_FS_RMDIR_ERROR_NOTEMPTY;
            case EACCES:
                return AVEN_FS_RMDIR_ERROR_ACCESS;
            case ENOENT:
                return AVEN_FS_RMDIR_ERROR_BADPATH;
#ifndef _WIN32
            case EBUSY:
                return AVEN_FS_RMDIR_ERROR_ACCESS;
            case EINVAL:
            case ENOTDIR:
                return AVEN_FS_RMDIR_ERROR_BADPATH;
#endif
            default:
                return AVEN_FS_RMDIR_ERROR_OTHER;
        }
    }

    return 0;
#endif
}

AVEN_FN int aven_fs_mkdir(AvenStr path) {
#ifdef _WIN32
    int error = _mkdir(path.ptr);
    if (error != 0) {
        switch (errno) {
            case EACCES:
                return AVEN_FS_MKDIR_ERROR_ACCESS;
            case ENOENT:
                return AVEN_FS_MKDIR_ERROR_BADPATH;
            case EEXIST:
                return AVEN_FS_MKDIR_ERROR_EXIST;
            default:
                return AVEN_FS_MKDIR_ERROR_OTHER;
        }
    }

    return 0;
#else
    int error = mkdir(
        path.ptr,
        S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH
    );
    if (error != 0) {
        switch (errno) {
            case EACCES:
                return AVEN_FS_MKDIR_ERROR_ACCESS;
            case ENOENT:
                return AVEN_FS_MKDIR_ERROR_BADPATH;
            case EEXIST:
                return AVEN_FS_MKDIR_ERROR_EXIST;
            case ENAMETOOLONG:
            case ENOTDIR:
                return AVEN_FS_MKDIR_ERROR_BADPATH;
            default:
                return AVEN_FS_MKDIR_ERROR_OTHER;
        }
    }

    return 0;
#endif
}

AVEN_FN int aven_fs_trunc(AvenStr path) {
#ifdef _WIN32
    int fd = _open(
        path.ptr,
        _O_CREAT | _O_TRUNC | _O_WRONLY,
        _S_IREAD | _S_IWRITE
    );
    if (fd < 0) {
        switch (errno) {
            case EACCES:
                return AVEN_FS_TRUNC_ERROR_ACCESS;
            case ENOENT:
                return AVEN_FS_TRUNC_ERROR_BADPATH;
            default:
                return AVEN_FS_TRUNC_ERROR_OTHER;
        }
    }

    _close(fd);

    return 0;
#else
    int fd = -1;
    do {
        fd = open(
            path.ptr,
            O_CREAT | O_TRUNC | O_WRONLY,
            S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH
        );
    } while (fd < 0 and errno == EINTR);
    if (fd < 0) {
        switch (errno) {
            case EACCES:
                return AVEN_FS_TRUNC_ERROR_ACCESS;
            case ENOENT:
                return AVEN_FS_TRUNC_ERROR_BADPATH;
            case ENOTDIR:
            case EISDIR:
                return AVEN_FS_TRUNC_ERROR_BADPATH;
            default:
                return AVEN_FS_TRUNC_ERROR_OTHER;
        }
    }

    close(fd);

    return 0;
#endif
}

AVEN_FN int aven_fs_copy(AvenStr ipath, AvenStr opath) {
#ifdef _WIN32
    AVEN_WIN32_FN(int) CopyFileA(
        const char *fname,
        const char *copy_fname,
        int fail_exists
    );
    AVEN_WIN32_FN(uint32_t) GetLastError(void);

    int success = CopyFileA(ipath.ptr, opath.ptr, false);
    if (success == 0) {
        switch (GetLastError()) {
            case 2: /* ERROR_FILE_NOT_FOUND */
                return AVEN_FS_COPY_ERROR_IFOPEN;
            case 3: /* ERROR_ACCESS_DENIED */
                return AVEN_FS_COPY_ERROR_OFOPEN;
            default:
                return AVEN_FS_COPY_ERROR_OTHER;
        }
    }

    return 0;
#else
    int ifd = -1;
    do {
        ifd = open(ipath.ptr, O_RDONLY, 0);
    } while (ifd < 0 and errno == EINTR);
    if (ifd < 0) {
        return AVEN_FS_COPY_ERROR_IFOPEN;
    }

    int ofd = -1;
    do {
        ofd = open(
            opath.ptr,
            O_CREAT | O_TRUNC | O_WRONLY,
            S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH
        );
    } while (ofd < 0 and errno == EINTR);
    if (ofd < 0) {
        return AVEN_FS_COPY_ERROR_OFOPEN;
    }

    char buffer[4096];
    ssize_t ilen = 0;
    do {
        do {
            ilen = read(ifd, buffer, sizeof(buffer));
        } while (ilen < 0 and errno == EINTR);
        if (ilen < 0) {
            return AVEN_FS_COPY_ERROR_IFREAD;
        }
        ssize_t written = 0;
        while (written < ilen) {
            ssize_t olen = write(
                ofd,
                &buffer[written],
                (size_t)(ilen - written)
            );
            if (olen >= 0) {
                written += olen;
            } else if (errno != EINTR) {
                return AVEN_FS_COPY_ERROR_OFWRITE;
            }
        }
    } while (ilen > 0);

    close(ifd);
    close(ofd);

    return 0;
#endif
}

AVEN_FN void aven_fs_utf8_mode(void) {
#ifdef _WIN32
    AVEN_WIN32_FN(int) SetConsoleOutputCP(unsigned int code_page_id);

    _setmode(0, _O_BINARY);
    _setmode(1, _O_BINARY);
    SetConsoleOutputCP(65001); /* CP_UTF8 */
#else
#endif
}

#if defined(_WIN32) and defined(_MSC_VER) and defined(__clang__)
    #pragma clang diagnostic pop
#endif

#endif // AVEN_IMPLEMENTATION

#endif // AVEN_FS_H
