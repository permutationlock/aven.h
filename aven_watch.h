#ifndef AVEN_WATCh_H
#define AVEN_WATCh_H

#ifdef _WIN32
    typedef void *AvenWatchHandle;

    #define AVEN_WATCH_INVALID_HANDLE ((AvenWatchHandle)-1L)

    AvenWatchHandle FindFirstChangeNotificationA(
        const char *path_name,
        int watch_subtree,
        uint32_t notify_filter
    );
    int FindNextChangeNotification(AvenWatchHandle handle);
    int FindCloseChangeNotification(AvenWatchHandle handle);
    uint32_t WaitForMultipleObjects(
        uint32_t ncount,
        const AvenWatchHandle *handles,
        int waith_all,
        uint32_t wait_milliseconds
    );

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
        return FindFirstChangeNotificationA(buffer, 0, 1);
    }

    bool aven_watch_check(AvenWatchHandle handle) {
        uint32_t result = WaitForMultipleObjects(1, &handle, 0, 0);

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
            IN_CREATE | IN_DELETE | IN_MOVED_FROM | IN_MOVED_TO
        );
        if (result <= 0) {
            return AVEN_WATCH_INVALID_HANDLE;
        }

        return handle;
    }

    bool aven_watch_check(AvenWatchHandle handle) {
        struct pollfd pfd = { .fd = handle, .events = POLLIN };
        int nevents = poll(&pfd, 1, 0);
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
