#ifndef AVEN_WATCh_H
#define AVEN_WATCh_H

#include <aven.h>

#define AVEN_WATCH_MAX_HANDLES 64

#ifdef _WIN32
    typedef void *AvenWatchHandle;
    typedef Slice(AvenWatchHandle) AvenWatchHandleSlice;

    #ifndef WIN_INFINITE
        #define WIN_INFINITE 0xffffffff
    #endif

    #define AVEN_WATCH_HANDLE_INVALID ((AvenWatchHandle)-1L)

    AvenWatchHandle FindFirstChangeNotificationA(
        const char *path_name,
        int watch_subtree,
        uint32_t notify_filter
    );
    int FindNextChangeNotification(AvenWatchHandle handle);
    int FindCloseChangeNotification(AvenWatchHandle handle);
    uint32_t WaitForMultipleObjects(
        uint32_t nhandles,
        AvenWatchHandle handle,
        int wait_all,
        uint32_t timeout_ms
    );

    AvenWatchHandle aven_watch_init(const char *dirname) {
        return FindFirstChangeNotificationA(dirname, 0, 0x1 | 0x2 | 0x8 | 0x10);
    }

    bool aven_watch_check_multiple(AvenWatchHandleSlice handles, int timeout) {
        uint32_t win_timeout = (uint32_t)timeout;
        if (timeout < 0) {
            win_timeout = WIN_INFINITE;
        }
        bool signaled = false;
        do {
            uint32_t result = WaitForMultipleObjects(
                (uint32_t)handles.len,
                handles.ptr,
                false,
                win_timeout
            );
            if (result >= handles.len) {
                return signaled;
            }

            signaled = true;
            win_timeout = 0;

            int success = FindNextChangeNotification(
                slice_get(handles, result)
            );
            assert(success != 0);

            handles.ptr += (result + 1);
            handles.len -= (result + 1);
        } while (handles.len > 0);

        return signaled;
    }

    bool aven_watch_check(AvenWatchHandle handle, int timeout) {
        AvenWatchHandleSlice handles = { .ptr = &handle, .len = 1 };
        return aven_watch_check_multiple(handles, timeout);
    }
 
    void aven_watch_deinit(AvenWatchHandle handle) {
        int success = FindCloseChangeNotification(handle);
        assert(success != 0);
    }
#else
    #include <poll.h>
    #include <sys/inotify.h>
    #include <unistd.h>

    typedef int AvenWatchHandle;
    typedef Slice(AvenWatchHandle) AvenWatchHandleSlice;

    #define AVEN_WATCH_HANDLE_INVALID -1

    AvenWatchHandle aven_watch_init(const char *dirname) {
        AvenWatchHandle handle = inotify_init();
        if (handle < 0) {
            return AVEN_WATCH_HANDLE_INVALID;
        }

        int result = inotify_add_watch(
            handle,
            dirname, 
            IN_CREATE | IN_DELETE | IN_MOVED_FROM | IN_MOVED_TO | IN_MODIFY
        );
        if (result <= 0) {
            return AVEN_WATCH_HANDLE_INVALID;
        }

        return handle;
    }

    bool aven_watch_check_multiple(AvenWatchHandleSlice handles, int timeout) {
        struct pollfd pfds[AVEN_WATCH_MAX_HANDLES];
        assert(handles.len < AVEN_WATCH_MAX_HANDLES);

        for (size_t i = 0; i < handles.len; i += 1) {
            pfds[i] = (struct pollfd){
                .fd = slice_get(handles, i),
                .events = POLLIN
            };
        }

        int nevents = poll(pfds, handles.len, timeout);
        assert(nevents >= 0);
        if (nevents <= 0) {
            return false;
        }

        for (size_t i = 0; i < handles.len; i += 1) {
            if ((pfds[i].revents & POLLIN) == 0) {
                continue;
            }

            unsigned char buffer[32 * sizeof(struct inotify_event)];
            int64_t len = read(slice_get(handles, i), buffer, sizeof(buffer));
            assert(len >= 0);
        }
        
        return true;
    }

    bool aven_watch_check(AvenWatchHandle handle, int timeout) {
        AvenWatchHandleSlice handles = { .ptr = &handle, .len = 1 };
        return aven_watch_check_multiple(handles, timeout);
    }

    void aven_watch_deinit(AvenWatchHandle handle) {
        close(handle);
    }
#endif

#endif // AVEN_WATCh_H
