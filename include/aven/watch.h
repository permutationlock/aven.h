#ifndef AVEN_WATCH_H
#define AVEN_WATCH_H

#include "../aven.h"
#include "str.h"

#define AVEN_WATCH_MAX_HANDLES 32

#ifdef _WIN32
    typedef void *AvenWatchHandle;
    typedef Slice(AvenWatchHandle) AvenWatchHandleSlice;

    #define AVEN_WATCH_HANDLE_INVALID ((AvenWatchHandle)-1L)
#else
    typedef int AvenWatchHandle;
    typedef Slice(AvenWatchHandle) AvenWatchHandleSlice;

    #define AVEN_WATCH_HANDLE_INVALID -1
#endif

typedef Result(uint32_t) AvenWatchResult;
typedef enum {
    AVEN_WATCH_ERROR_NONE = 0,
    AVEN_WATCH_ERROR_FILE,
    AVEN_WATCH_ERROR_POLL,
    AVEN_WATCH_ERROR_UNSUPPORTED,
} AvenWatchError;

AVEN_FN AvenWatchHandle aven_watch_init(AvenStr dirname);
AVEN_FN AvenWatchResult aven_watch_check_multiple(
    AvenWatchHandleSlice handles,
    int timeout
);
AVEN_FN AvenWatchResult aven_watch_check(AvenWatchHandle handle, int timeout);
AVEN_FN void aven_watch_deinit(AvenWatchHandle handle);

#ifdef AVEN_IMPLEMENTATION

#ifdef _WIN32
    AVEN_FN AvenWatchHandle aven_watch_init(AvenStr dirname) {
        AVEN_WIN32_FN(AvenWatchHandle) FindFirstChangeNotificationA(
            const char *path_name,
            int watch_subtree,
            uint32_t notify_filter
        );

        return FindFirstChangeNotificationA(
            dirname.ptr,
            0,
            0x1 | 0x2 | 0x8 | 0x10
        );
    }

    AVEN_FN AvenWatchResult aven_watch_check_multiple(
        AvenWatchHandleSlice handles,
        int timeout
    ) {
        AVEN_WIN32_FN(uint32_t) WaitForMultipleObjects(
            uint32_t nhandles,
            AvenWatchHandle handle,
            int wait_all,
            uint32_t timeout_ms
        );
        AVEN_WIN32_FN(int) FindNextChangeNotification(
            AvenWatchHandle handle
        );

        assert(handles.len < AVEN_WATCH_MAX_HANDLES);
        uint32_t win_timeout = (uint32_t)timeout;
        if (timeout < 0) {
            win_timeout = 0xffffffff; /* INFINITE */
        }
        uint32_t signaled = 0;
        do {
            uint32_t result = WaitForMultipleObjects(
                (uint32_t)handles.len,
                handles.ptr,
                false,
                win_timeout
            );
            if (result == 0x00000102 /* TIMEOUT */) {
                return (AvenWatchResult){ .payload = signaled };
            } else if (result >= handles.len) {
                return (AvenWatchResult){ .error = AVEN_WATCH_ERROR_POLL };
            }

            signaled |= ((uint32_t)1) << result;
            win_timeout = 0;

            int success = FindNextChangeNotification(
                slice_get(handles, result)
            );
            if (success == 0) {
                return (AvenWatchResult){ .error = AVEN_WATCH_ERROR_FILE };
            }

            handles.ptr += (result + 1);
            handles.len -= (result + 1);
        } while (handles.len > 0);

        return (AvenWatchResult){ .payload = signaled };
    }

    AVEN_FN AvenWatchResult aven_watch_check(
        AvenWatchHandle handle,
        int timeout
    ) {
        AvenWatchHandleSlice handles = { .ptr = &handle, .len = 1 };
        return aven_watch_check_multiple(handles, timeout);
    }
 
    AVEN_FN void aven_watch_deinit(AvenWatchHandle handle) {
        AVEN_WIN32_FN(int) FindCloseChangeNotification(
            AvenWatchHandle handle
        );

        FindCloseChangeNotification(handle);
    }
#elif defined(__linux__)
    #include <errno.h>

    #include <poll.h>
    #include <sys/inotify.h>
    #include <unistd.h>

    AVEN_FN AvenWatchHandle aven_watch_init(AvenStr dirname) {
        AvenWatchHandle handle = inotify_init();
        if (handle < 0) {
            return AVEN_WATCH_HANDLE_INVALID;
        }

        int result = inotify_add_watch(
            handle,
            dirname.ptr, 
            IN_CREATE | IN_DELETE | IN_MOVED_FROM | IN_MOVED_TO | IN_MODIFY
        );
        if (result <= 0) {
            return AVEN_WATCH_HANDLE_INVALID;
        }

        return handle;
    }

    AVEN_FN AvenWatchResult aven_watch_check_multiple(
        AvenWatchHandleSlice handles,
        int timeout
    ) {
        assert(handles.len < AVEN_WATCH_MAX_HANDLES);

        struct pollfd pfds[AVEN_WATCH_MAX_HANDLES];
        for (size_t i = 0; i < handles.len; i += 1) {
            pfds[i] = (struct pollfd){
                .fd = slice_get(handles, i),
                .events = POLLIN
            };
        }

        int nevents = poll(pfds, handles.len, timeout);
        if (nevents == 0) {
            return (AvenWatchResult){ .payload = false };
        } else if (nevents < 0) {
            return (AvenWatchResult){ .error = AVEN_WATCH_ERROR_POLL };
        }

        uint32_t signaled = 0;
        for (size_t i = 0; i < handles.len; i += 1) {
            if ((pfds[i].revents & POLLIN) == 0) {
                continue;
            }

            signaled |= ((uint32_t)1) << i;


            unsigned char buffer[32 * sizeof(struct inotify_event)];

            ssize_t len = 0;
            do {
                len = read(
                    slice_get(handles, i),
                    buffer,
                    sizeof(buffer)
                );
                if (len < 0 and errno != EINTR) {
                    return (AvenWatchResult){ .error = AVEN_WATCH_ERROR_FILE };
                }
            } while (len < 0);
        }
        
        return (AvenWatchResult){ .payload = signaled };
    }

    AVEN_FN AvenWatchResult aven_watch_check(
        AvenWatchHandle handle,
        int timeout
    ) {
        AvenWatchHandleSlice handles = { .ptr = &handle, .len = 1 };
        return aven_watch_check_multiple(handles, timeout);
    }

    AVEN_FN void aven_watch_deinit(AvenWatchHandle handle) {
        close(handle);
    }
#else
    AVEN_FN AvenWatchHandle aven_watch_init(AvenStr dirname) {
        (void)dirname;
        return AVEN_WATCH_HANDLE_INVALID;
    }

    AVEN_FN AvenWatchResult aven_watch_check_multiple(
        AvenWatchHandleSlice handles,
        int timeout
    ) {
        (void)handles;
        (void)timeout;
        return (AvenWatchResult){ .error = AVEN_WATCH_ERROR_UNSUPPORTED };
    }

    AVEN_FN AvenWatchResult aven_watch_check(
        AvenWatchHandle handle,
        int timeout
    ) {
        (void)handle;
        (void)timeout;
        return (AvenWatchResult){ .error = AVEN_WATCH_ERROR_UNSUPPORTED };
    }

    AVEN_FN void aven_watch_deinit(AvenWatchHandle handle) {
        (void)handle;
    }
#endif

#endif // AVEN_IMPLEMENTATION

#endif // AVEN_WATCH_H
