#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
    #define _POSIX_C_SOURCE 200112L
#endif
#define AVEN_IMPLEMENTATION
#define AVEN_IMPLEMENTATION_STU
#include <aven/arena.h>
#include <aven/arg.h>
#include <aven/build.h>
#include <aven/dl.h>
#include <aven/fs.h>
#include <aven/path.h>
#include <aven/test.h>
#include <aven/watch.h>

