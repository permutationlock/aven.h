#ifndef AVEN_TIME_H
#define AVEN_TIME_H

#include "aven.h"

#if _POSIX_C_SOURCE < 199309L
    #error "requires _POSIX_C_SOURCE >= 199309L"
#endif

#include <time.h>

typedef struct timespec AvenTimeSpec;

static inline int64_t aven_time_since(AvenTimeSpec end, AvenTimeSpec start) {
    int64_t seconds = (int64_t)end.tv_sec - (int64_t)start.tv_sec;
    int64_t sec_diff = seconds * 1000L * 1000L * 1000L;
    int64_t nsec_diff = (int64_t)end.tv_nsec - (int64_t)start.tv_nsec;
    return sec_diff + nsec_diff;
}

#endif // AVEN_TIME_H
