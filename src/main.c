#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <aven.h>
#include <aven/arena.h>
#include <aven/dl.h>
#include <aven/path.h>
#include <aven/str.h>
#include <aven/watch.h>

int collatz(int n);
int fibonacci(int n);

int main(void) {
    void *mem = malloc(4096);
    assert(mem != NULL);

    AvenArena arena = aven_arena_init(mem, 4096);

    printf("FunMath:\n");
    printf("\tcollatz(%d) = %d\n", 5, collatz(5));
    printf("\tfibonacci(%d) = %d\n", 5, fibonacci(5));

    char *cwd_path = aven_path_exe_dir(&arena);

    char *deplock_path = aven_path(&arena, cwd_path, "deplock", NULL);
    char *libhot_path = aven_path(&arena, cwd_path, "dep", "libhot", NULL);

    AvenWatchHandle lock_handle = aven_watch_init(deplock_path);
    if (lock_handle == AVEN_WATCH_HANDLE_INVALID) {
        fprintf(stderr, "failed to watch %s", deplock_path);
        return 1;
    }

    for (;;) {
        void *libhot = aven_dl_open(libhot_path);
        if (libhot != NULL) {
            char *hot_message = aven_dl_sym(libhot, "message");
            if (hot_message != NULL) {
                printf("%s\n", hot_message);
            }
            aven_dl_close(libhot);
        } else {
            fprintf(stderr, "failed to load %s: %d\n", libhot_path, errno);
        }
        aven_watch_check(lock_handle, -1);
    }

    return 0;
}

