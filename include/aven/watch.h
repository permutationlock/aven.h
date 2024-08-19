#ifndef AVEN_WATCh_H
#define AVEN_WATCh_H

#include <aven.h>

#ifdef _WIN32
    typedef void *AvenWatchHandle;

    #ifndef WIN_INFINITE
        #define WIN_INFINITE 0xffffffff
    #endif

    #define AVEN_WATCH_INVALID_HANDLE ((AvenWatchHandle)-1L)

    AvenWatchHandle FindFirstChangeNotificationA(
        const char *path_name,
        int watch_subtree,
        uint32_t notify_filter
    );
    int FindNextChangeNotification(AvenWatchHandle handle);
    int FindCloseChangeNotification(AvenWatchHandle handle);
    uint32_t WaitForSingleObject(AvenWatchHandle handle, uint32_t timeout_ms);

    AvenWatchHandle aven_watch_init(const char *dirname) {
        char buffer[1024];
        size_t i;
        for (i = 0; dirname[i] != '\0' and i < countof(buffer) - 1; ++i) {
            if (dirname[i] == '/') {
                buffer[i] = '\\';
            } else {
                buffer[i] = dirname[i];
            }
        }
        buffer[i] = '\0';
        return FindFirstChangeNotificationA(buffer, 0, 0x1 | 0x2 | 0x8 | 0x10);
    }

    bool aven_watch_check(AvenWatchHandle handle, int timeout) {
        uint32_t win_timeout = (uint32_t)timeout;
        if (timeout < 0) {
            win_timeout = WIN_INFINITE;
        }
        uint32_t result = WaitForSingleObject(handle, win_timeout);

        int success = FindNextChangeNotification(handle);
        assert(success != 0);

        return (result == 0);
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

    #define AVEN_WATCH_INVALID_HANDLE -1

    AvenWatchHandle aven_watch_init(const char *dirname) {
        AvenWatchHandle handle = inotify_init();
        if (handle < 0) {
            return AVEN_WATCH_INVALID_HANDLE;
        }

        int result = inotify_add_watch(
            handle,
            dirname, 
            IN_CREATE | IN_DELETE | IN_MOVED_FROM | IN_MOVED_TO | IN_MODIFY
        );
        if (result <= 0) {
            return AVEN_WATCH_INVALID_HANDLE;
        }

        return handle;
    }

    bool aven_watch_check(AvenWatchHandle handle, int timeout) {
        struct pollfd pfd = { .fd = handle, .events = POLLIN };
        int nevents = poll(&pfd, 1, timeout);
        assert(nevents >= 0);
        if (nevents <= 0) {
            return false;
        }

        unsigned char buffer[32 * sizeof(struct inotify_event)];
        int64_t len = read(handle, buffer, sizeof(buffer));
        assert(len >= 0);
        
        return true;
    }

    void aven_watch_deinit(AvenWatchHandle handle) {
        close(handle);
    }
#endif

#endif // AVEN_WATCh_H
