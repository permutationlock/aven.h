#define _POSIX_C_SOURCE 200809L

#include <stdlib.h>

#define AVEN_IMPLEMENTATION

#include "aven/include/aven.h"
#include "aven/include/aven/arena.h"
#include "aven/include/aven/arg.h"
#include "aven/include/aven/build.h"
#include "aven/include/aven/path.h"
#include "aven/include/aven/watch.h"

#include "build_common.h"

AvenArg custom_args[] = {
    {
        .name = "watch",
        .description = "Automatically re-build on changes to files in src/",
        .type = AVEN_ARG_TYPE_BOOL,
    },
};

AvenArg args[countof(custom_args) + countof(default_args)];
 
#define ARENA_SIZE (4096 * 2000)

int main(int argc, char **argv) {
    void *mem = malloc(ARENA_SIZE);
    assert(mem != NULL);

    AvenArena arena = aven_arena_init(mem, ARENA_SIZE);

    AvenArgSlice arg_slice = { .len = countof(args), .ptr = args };
    memcpy(arg_slice.ptr, custom_args, sizeof(custom_args));
    memcpy(
        arg_slice.ptr + countof(custom_args),
        default_args,
        sizeof(default_args)
    );

    int error = aven_arg_parse(arg_slice, argv, argc);
    if (error != 0) {
        return error;
    }

    DefaultOpts opts = get_default_opts(arg_slice, &arena);
    bool watch = aven_arg_get_bool(arg_slice, "watch");

    char *build_dir = "build_work";
    char *out_dir = "build_out";
    char *src_dir = "src";
    char *libhot_dir = aven_path(&arena, src_dir, "libhot", NULL);
    char *exe_fname = "print_funmath";

    AvenBuildStep build_dir_step = aven_build_step_mkdir(build_dir);
    AvenBuildStep out_dir_step = aven_build_step_mkdir(out_dir);

    char *inc_paths[] = { aven_path(&arena, "aven", "include", NULL) };
    CStrSlice includes = { .ptr = inc_paths, .len = countof(inc_paths) };
    CStrSlice macros = { 0 };

    AvenBuildStep bin_dir_step;
    {
        char *dir_path = aven_path(&arena, out_dir, "bin", NULL);
        bin_dir_step = aven_build_step_mkdir(dir_path);
        aven_build_step_add_dep(&bin_dir_step, &out_dir_step, &arena);
    }

    AvenBuildStep dep_dir_step;
    {
        char *dir_path = aven_path(
            &arena,
            bin_dir_step.out_path.value,
            "dep",
            NULL
        );
        dep_dir_step = aven_build_step_mkdir(dir_path);
        aven_build_step_add_dep(&dep_dir_step, &bin_dir_step, &arena);
    }

    AvenBuildStep lock_dir_step;
    {
        char *dir_path = aven_path(
            &arena,
            bin_dir_step.out_path.value,
            "deplock",
            NULL
        );
        lock_dir_step = aven_build_step_mkdir(dir_path);
        aven_build_step_add_dep(&lock_dir_step, &bin_dir_step, &arena);
    }

    AvenBuildStep aven_dir_step;
    {
        char *dir_path = aven_path(
            &arena,
            build_dir_step.out_path.value,
            "aven",
            NULL
        );
        aven_dir_step = aven_build_step_mkdir(dir_path);
        aven_build_step_add_dep(&aven_dir_step, &build_dir_step, &arena);
    }

    AvenBuildStep aven_lib_step = build_aven_step(
        &opts,
        "aven/include",
        "aven/src",
        &aven_dir_step,
        &build_dir_step,
        &arena
    );

    AvenBuildStep main_obj_step;
    {
        char *src_path = aven_path(&arena, src_dir, "main.c", NULL);
        char *obj_path = aven_path(&arena, build_dir, "main.o", NULL);
        main_obj_step = cc_compile_obj_ex(
            &opts,
            includes,
            macros,
            src_path,
            obj_path,
            &arena
        );
        aven_build_step_add_dep(&main_obj_step, &build_dir_step, &arena);
    }

    AvenBuildStep collatz_obj_step;
    {
        char *src_path = aven_path(&arena, src_dir, "collatz.c", NULL);
        char *obj_path = aven_path(&arena, build_dir, "collatz.o", NULL);
        collatz_obj_step = cc_compile_obj(&opts, src_path, obj_path, &arena);
        aven_build_step_add_dep(&collatz_obj_step, &build_dir_step, &arena);
    }

    AvenBuildStep fibonacci_obj_step;
    {
        char *src_path = aven_path(&arena, src_dir, "fibonacci.c", NULL);
        char *obj_path = aven_path(&arena, build_dir, "fibonacci.o", NULL);
        fibonacci_obj_step = cc_compile_obj(&opts, src_path, obj_path, &arena);
        aven_build_step_add_dep(&fibonacci_obj_step, &build_dir_step, &arena);
    }

    AvenBuildStep lib_step;
    {
        AvenBuildStep *dep_objs[] = {
            &collatz_obj_step,
            &fibonacci_obj_step,
        };
        AvenBuildStepPtrSlice deps = {
            .ptr = dep_objs,
            .len = countof(dep_objs),
        };
        char *lib_path = aven_path(&arena, build_dir, "funmath", NULL);
        lib_step = ar_create_lib(&opts, deps, lib_path, &arena);
        aven_build_step_add_dep(&lib_step, &build_dir_step, &arena);
    }

    AvenBuildStep hot_obj_step;
    {
        char *src_path = aven_path(&arena, libhot_dir, "hot.c", NULL);
        char *obj_path = aven_path(&arena, build_dir, "hot.o", NULL);
        hot_obj_step = cc_compile_obj(&opts, src_path, obj_path, &arena);
        aven_build_step_add_dep(&hot_obj_step, &build_dir_step, &arena);
    }

    AvenBuildStep build_dep_step;
    {
        char *lock_path = aven_path(
            &arena,
            lock_dir_step.out_path.value,
            "lock",
            NULL
        );
        build_dep_step = aven_build_step_touch(lock_path);
        aven_build_step_add_dep(&build_dep_step, &lock_dir_step, &arena);
    }

    AvenBuildStep link_so_step;
    {
        AvenBuildStep *dep_objs[] = { &hot_obj_step };
        AvenBuildStepPtrSlice deps = {
            .ptr = dep_objs,
            .len = countof(dep_objs),
        };
        char *lib_path = aven_path(
            &arena,
            dep_dir_step.out_path.value,
            "libhot",
            NULL
        );
        link_so_step = ld_link_so(&opts, deps, lib_path, &arena);
        aven_build_step_add_dep(&link_so_step, &dep_dir_step, &arena);
    }
    aven_build_step_add_dep(&build_dep_step, &link_so_step, &arena);

    AvenBuildStep link_exe_step;
    {
        AvenBuildStep *dep_objs[] = {
            &main_obj_step,
            &aven_lib_step,
            &lib_step,
        };
        AvenBuildStepPtrSlice deps = {
            .ptr = dep_objs,
            .len = countof(dep_objs),
        };
        char *exe_path = aven_path(
            &arena,
            bin_dir_step.out_path.value,
            exe_fname,
            NULL
        );
        link_exe_step = ld_link_exe(&opts, deps, exe_path, &arena);
        aven_build_step_add_dep(&link_exe_step, &bin_dir_step, &arena);
    }

    AvenBuildStep root_step = aven_build_step_root();
    aven_build_step_add_dep(&root_step, &link_exe_step, &arena);
    aven_build_step_add_dep(&root_step, &build_dep_step, &arena);

    if (opts.clean) {
        aven_build_step_clean(&root_step);
    } else if (watch) {
        AvenWatchHandle src_handle = aven_watch_init(src_dir);
        if (src_handle == AVEN_WATCH_HANDLE_INVALID) {
            fprintf(stderr, "couldn't watch %s\n", src_dir);
        }
        AvenWatchHandle libhot_handle = aven_watch_init(libhot_dir);
        if (libhot_handle == AVEN_WATCH_HANDLE_INVALID) {
            fprintf(stderr, "couldn't watch %s\n", libhot_dir);
        }
        AvenWatchHandle handles[] = { src_handle, libhot_handle };
        AvenWatchHandleSlice handle_slice = {
            .ptr = handles,
            .len = countof(handles),
        };
        for (;;) {
            error = aven_build_step_run(&root_step, arena);
            if (error != 0) {
                fprintf(stderr, "BUILD ERROR: %d\n", error);
            }
            aven_build_step_reset(&root_step);
            bool success = aven_watch_check_multiple(handle_slice, -1);
            if (!success) {
                fprintf(stderr, "directory watching failed\n");
                error = 1;
                break;
            }
            while (aven_watch_check_multiple(handle_slice, 100)) {}
            printf("\nRE-BUILDING:\n");
        }
        aven_watch_deinit(libhot_handle);
        aven_watch_deinit(src_handle);
    } else {
        error = aven_build_step_run(&root_step, arena);
        if (error != 0) {
            fprintf(stderr, "BUILD ERROR: %d\n", error);
        }
    }

    free(mem);

    return error;
}

