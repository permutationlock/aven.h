#define _POSIX_C_SOURCE 200112L
#define AVEN_IMPLEMENTATION

#include <aven.h>
#include <aven/fs.h>
#include <aven/path.h>
#include <aven/str.h>
#include <aven/test.h>

#include <stdlib.h>

#include "test/path.c"
#include "test/build_common.c"

#define ARENA_SIZE (4096 * 16)

AvenArena test_arena;

int main(void) {
    aven_fs_utf8_mode();
    void *mem = malloc(ARENA_SIZE);
    test_arena = aven_arena_init(mem, ARENA_SIZE);

    test_path();
    test_build_common();

    return 0;
}
