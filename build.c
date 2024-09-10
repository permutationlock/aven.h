#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
    #define _POSIX_C_SOURCE 200112L
#endif

#include "config.h"

#define AVEN_IMPLEMENTATION

#include "include/aven.h"
#include "include/aven/arena.h"
#include "include/aven/build.h"
#include "include/aven/build/common.h"
#include "include/aven/fs.h"
#include "include/aven/str.h"

#include "build.h"

#include <stdio.h>
#include <stdlib.h>

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

    // Build the library

    AvenBuildStep out_dir_step = aven_build_step_mkdir(aven_str("build_out"));

    AvenBuildCommonOpts opts = aven_build_common_opts(
        aven_build_common_args,
        &arena
    );
    AvenBuildStep libaven_step = libaven_build_step(
        &opts,
        aven_str("."),
        &out_dir_step,
        &arena
    );

    AvenBuildStep root_step = aven_build_step_root();
    aven_build_step_add_dep(&root_step, &libaven_step, &arena);

    // Build and run tests

    AvenStr aven_include = libaven_build_include_path(aven_str("."), &arena);
    AvenBuildStep test_dir_step = aven_build_step_mkdir(aven_str("build_test"));

#ifdef _WIN32
    // If on windows build a manifest resource file to enable utf8 mode
    AvenBuildStep manifest_step = libaven_build_windres_manifest_step(
        &opts,
        aven_str("."),
        &test_dir_step,
        &arena
    );
    AvenBuildStep *test_obj_step_data[] = { &manifest_step };
    AvenBuildStepPtrSlice test_obj_steps = {
        .ptr = test_obj_step_data,
        .len = countof(test_obj_step_data),
    };
#else
    AvenBuildStepPtrSlice test_obj_steps = { 0 };
#endif

    AvenBuildStep test_step = aven_build_common_step_cc_ld_run_exe_ex(
        &opts,
        (AvenStrSlice){ .ptr = &aven_include, .len = 1 },
        (AvenStrSlice){ 0 },
        (AvenStrSlice){ 0 },
        test_obj_steps,
        aven_str("test.c"),
        &test_dir_step,
        false,
        (AvenStrSlice){ 0 },
        &arena
    );

    AvenBuildStep test_root_step = aven_build_step_root();
    aven_build_step_add_dep(&test_root_step, &test_step, &arena);

    // Execute the chosen build step

    if (opts.clean) {
        aven_build_step_clean(&root_step);
        aven_build_step_clean(&test_root_step);
    } else if (opts.test) {
        error = aven_build_step_run(&test_root_step, arena);
        if (error != 0) {
            fprintf(stderr, "TEST FAILED\n");
        }
    } else {
        error = aven_build_step_run(&root_step, arena);
        if (error != 0) {
            fprintf(stderr, "BUILD FAILED\n");
        }
    }

    return error;
}

