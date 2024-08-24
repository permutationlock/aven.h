#ifndef _WIN32
    #define _POSIX_C_SOURCE 200112L
#endif

#include <stdio.h>
#include <stdlib.h>

#define AVEN_IMPLEMENTATION

#include "include/aven.h"
#include "include/aven/arena.h"
#include "include/aven/build.h"
#include "include/aven/build/common.h"
#include "include/aven/str.h"

#include "build.h"

#define ARENA_SIZE (4096 * 2000)

int main(int argc, char **argv) {
    void *mem = malloc(ARENA_SIZE);
    if (mem == NULL) {
        fprintf(stderr, "malloc failure\n");
    }

    AvenArena arena = aven_arena_init(mem, ARENA_SIZE);

    int error = aven_arg_parse(
        aven_build_common_args,
        argv,
        argc,
        aven_build_common_overview.ptr,
        aven_build_common_usage.ptr
    );
    if (error != 0) {
        if (error != AVEN_ARG_ERROR_HELP) {
            fprintf(stderr, "ARG PARSE ERROR: %d\n", error);
            return error;
        }
        return 0;
    }

    AvenBuildStep work_dir_step = aven_build_step_mkdir(aven_str("build_work"));
    AvenBuildStep out_dir_step = aven_build_step_mkdir(aven_str("build_out"));

    AvenBuildCommonOpts opts = aven_build_common_opts(
        aven_build_common_args,
        &arena
    );
    AvenBuildStep root_step = libaven_build_step(
        &opts,
        aven_str("."),
        &work_dir_step,
        &out_dir_step,
        &arena
    );

    if (opts.clean) {
        aven_build_step_clean(&root_step);
    } else {
        error = aven_build_step_run(&root_step, arena);
        if (error != 0) {
            fprintf(stderr, "BUILD ERROR: %d\n", error);
        }
    }

    return error;
}

