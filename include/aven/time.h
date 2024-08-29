#ifndef AVEN_TIME_H
#define AVEN_TIME_H

#include "../aven.h"

#include <time.h>

#define AVEN_TIME_NSEC_IN_SEC (1000L * 1000L * 1000L)
#define AVEN_TIME_USEC_IN_SEC (1000L * 1000L)
#define AVEN_TIME_MSEC_IN_SEC (1000L)

typedef struct timespec AvenTimeInst;

AVEN_FN AvenTimeInst aven_time_now(void);
AVEN_FN int64_t aven_time_since(AvenTimeInst end, AvenTimeInst start);

#ifdef AVEN_IMPLEMENTATION

#ifdef _WIN32
    int QueryPerformanceFrequency(int64_t *freq);
    int QueryPerformanceCounter(int64_t *count);

    AVEN_FN AvenTimeInst aven_time_now(void) {
        int64_t freq;
        int success = QueryPerformanceFrequency(&freq);
        assert(success != 0);

        int64_t count;
        success = QueryPerformanceCounter(&count);
        assert(success != 0);

        AvenTimeInst now = {
            .tv_sec = count / freq,
            .tv_nsec = (int)(
                ((count % freq) * AVEN_TIME_NSEC_IN_SEC + (freq >> 1)) / freq
            ),
        };
        if (now->tv_nsec >= AVEN_TIME_NSEC_IN_SEC) {
            now.tv_sec += 1;
            now.tv_nsec -= AVEN_TIME_NSEC_IN_SEC;
        }

        return now;
    }
#else
    #if !defined(_POSIX_C_SOURCE) or _POSIX_C_SOURCE < 199309L
        #error "clock_gettime requires _POSIX_C_SOURCE >= 199309L"
    #endif
    #include <unistd.h>
    
    AVEN_FN AvenTimeInst aven_time_now(void) {
        AvenTimeInst now;
        int error = clock_gettime(CLOCK_MONOTONIC, &now);
        assert(error == 0);
        return now;
    }
#endif

AVEN_FN int64_t aven_time_since(AvenTimeInst end, AvenTimeInst start) {
    int64_t seconds = (int64_t)end->tv_sec - (int64_t)start->tv_sec;
    int64_t sec_diff = seconds * AVEN_TIME_NSEC_IN_SEC;
    int64_t nsec_diff = (int64_t)end->tv_nsec - (int64_t)start->tv_nsec;
    return sec_diff + nsec_diff;
}

#endif // AVEN_IMPLEMENTATION

#endif // AVEN_TIME_H
