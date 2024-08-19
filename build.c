#define _POSIX_C_SOURCE 200809L

#include <stdlib.h>

#include <aven.h>
#include <aven/arg.h>
#include <aven/build.h>
#include <aven/watch.h>

AvenArg build_args[] = {
    {
        .name = "-cc",
        .description = "C compiler exe",
        .type = AVEN_ARG_TYPE_STRING,
        .value = {
            .type = AVEN_ARG_TYPE_STRING,
#ifdef _WIN32
            .data = { .arg_string = "gcc.exe" },
#else
            .data = { .arg_string = "gcc" },
#endif
        },
    },
    {
        .name = "-cflags",
        .description = "C compiler common flags",
        .type = AVEN_ARG_TYPE_STRING,
        .value = {
            .type = AVEN_ARG_TYPE_STRING,
            .data = { .arg_string = "" },
        },
    },
    {
        .name = "-cobjflag",
        .description = "C compiler compile object flag",
        .type = AVEN_ARG_TYPE_STRING,
        .value = {
            .type = AVEN_ARG_TYPE_STRING,
            .data = { .arg_string = "-c" },
        },
    },
    {
        .name = "-coutflag",
        .description = "C compiler output filename flag",
        .type = AVEN_ARG_TYPE_STRING,
        .value = {
            .type = AVEN_ARG_TYPE_STRING,
            .data = { .arg_string = "-o" },
        },
    },
    {
        .name = "-ld",
        .description = "Linker exe to use instead of C compiler",
        .type = AVEN_ARG_TYPE_STRING,
        .optional = true,
    },
    {
        .name = "-ldflags",
        .description = "Linker common flags",
        .type = AVEN_ARG_TYPE_STRING,
        .value = {
            .type = AVEN_ARG_TYPE_STRING,
            .data = { .arg_string = "" },
        },
    },
    {
        .name = "-ldoutflag",
        .description = "Linker output filename flag",
        .type = AVEN_ARG_TYPE_STRING,
        .value = {
            .type = AVEN_ARG_TYPE_STRING,
            .data = { .arg_string = "-o" },
        },
    },
#ifdef _WIN32
    {
        .name = "-posix",
        .description = "Use empty exe extension, \'.so\' shared lib extension",
        .type = AVEN_ARG_TYPE_BOOL,
    },
#else
    {
        .name = "-windows",
        .description =
            "Use \'.exe\' exe extension, \'.dll\' shared lib extension",
        .type = AVEN_ARG_TYPE_BOOL,
    },
#endif
    {
        .name = "-clean",
        .description = "Remove all build artifacts",
        .type = AVEN_ARG_TYPE_BOOL,
    },
    {
        .name = "-watch",
        .description = "Automatically e-build on changes to src/",
        .type = AVEN_ARG_TYPE_BOOL,
    },
};

typedef struct {
    char *compiler;
    char *objflag;
    char *outflag;
    AvenStrSlice flags;
} COpts;

AvenBuildStep cc_compile_obj(
    COpts *copts,
    char *src_path,
    char *target_path,
    AvenArena *arena
) {
    AvenBuildCmdSlice cmd_slice = { .len = copts->flags.len + 5 };
    cmd_slice.ptr = aven_arena_create_array(char *, arena, cmd_slice.len);
    assert(cmd_slice.ptr != NULL);

    size_t i = 0;
    slice_get(cmd_slice, i) = copts->compiler;
    i += 1;

    for (size_t j = 0; j < copts->flags.len; j += 1) {    
        slice_get(cmd_slice, i) = slice_get(copts->flags, j).ptr;
        i += 1;
    }

    slice_get(cmd_slice, i) = copts->objflag;
    i += 1;
    slice_get(cmd_slice, i) = copts->outflag;
    i += 1;
    slice_get(cmd_slice, i) = target_path;
    i += 1;
    slice_get(cmd_slice, i) = src_path;
    i += 1;

    AvenBuildOptionalPath out_path = { .value = target_path, .valid = true };
    return aven_build_step_cmd_from_slice(out_path, cmd_slice);
}

typedef struct {
    char *linker;
    char *outflag;
    AvenStrSlice flags;
} LDOpts;

typedef Slice(AvenBuildStep *) AvenBuildStepPtrSlice;

AvenBuildStep ld_link_exe(
    LDOpts *ldopts,
    AvenBuildStepPtrSlice obj_steps,
    char *target_path,
    AvenArena *arena
) {
    AvenBuildCmdSlice cmd_slice = {
        .len = ldopts->flags.len + obj_steps.len + 3
    };
    cmd_slice.ptr = aven_arena_create_array(char *, arena, cmd_slice.len);
    assert(cmd_slice.ptr != NULL);

    size_t i = 0;
    slice_get(cmd_slice, i) = ldopts->linker;
    i += 1;

    for (size_t j = 0; j < ldopts->flags.len; j += 1) {
        slice_get(cmd_slice, i) = slice_get(ldopts->flags, j).ptr;
        i += 1;
    }

    slice_get(cmd_slice, i) = ldopts->outflag;
    i += 1;
    slice_get(cmd_slice, i) = target_path;
    i += 1;

    for (size_t j = 0; j < obj_steps.len; j += 1) {
        AvenBuildStep *obj_step = slice_get(obj_steps, j);
        assert(obj_step->out_path.valid);
        slice_get(cmd_slice, i) = obj_step->out_path.value;
        i += 1;
    }

    AvenBuildOptionalPath out_path = { .value = target_path, .valid = true };
    AvenBuildStep link_step = aven_build_step_cmd_from_slice(
        out_path,
        cmd_slice
    );

    for (size_t j = 0; j < obj_steps.len; j += 1) {
        aven_build_step_add_dep(&link_step, slice_get(obj_steps, j), arena);
    }

    return link_step;
}

#define ARENA_SIZE (4096 * 2000)

int main(int argc, char **argv) {
    void *mem = malloc(ARENA_SIZE);
    assert(mem != NULL);

    AvenArena arena = aven_arena_init(mem, ARENA_SIZE);

    int error = aven_arg_parse(
        build_args,
        countof(build_args),
        argv,
        argc
    );
    if (error != 0) {
        return error;
    }

    AvenArgSlice arg_slice = { .ptr = build_args, .len = countof(build_args) };

    COpts copts = { 0 };
    copts.compiler = aven_arg_get_string(arg_slice, "-cc");
    copts.objflag = aven_arg_get_string(arg_slice, "-cobjflag");
    copts.outflag = aven_arg_get_string(arg_slice, "-coutflag");
    {
        char *cflags_raw = aven_arg_get_string(arg_slice, "-cflags");
        AvenStrSliceResult result = aven_str_split(
            aven_str_from_cstr(cflags_raw),
            ' ',
            &arena
        );
        assert(result.error == 0);

        copts.flags = result.payload;
    }

    LDOpts ldopts = { 0 };
    if (aven_arg_has_arg(arg_slice, "-ld")) {
        ldopts.linker = aven_arg_get_string(arg_slice, "-ld");
        ldopts.outflag = aven_arg_get_string(arg_slice, "-ldoutflag");
    } else {
        ldopts.linker = copts.compiler;
        ldopts.outflag = copts.outflag;
    }
    {
        char *ldflags_raw = aven_arg_get_string(arg_slice, "-ldflags");
        AvenStrSliceResult result = aven_str_split(
            aven_str_from_cstr(ldflags_raw),
            ' ',
            &arena
        );
        assert(result.error == 0);

        ldopts.flags = result.payload;
    }

#ifdef _WIN32
    bool windows_build = !aven_arg_get_bool(arg_slice, "-posix");
#else
    bool windows_build = aven_arg_get_bool(arg_slice, "-windows");
#endif

    char *build_dir = "build_temp";
    char *out_dir = "build_out";
    char *bin_dir = "bin";
    char *src_dir = "src";
    char *exe_fname;
    if (windows_build) {
        exe_fname = "print_collatz.exe";
    } else {
        exe_fname = "print_collatz";
    }

    AvenBuildStep build_dir_step = aven_build_step_mkdir(build_dir);
    AvenBuildStep out_dir_step = aven_build_step_mkdir(out_dir);

    AvenBuildStep bin_dir_step;
    {
        char *bin_path = aven_build_path(&arena, out_dir, bin_dir, NULL);
        bin_dir_step = aven_build_step_mkdir(bin_path);
        aven_build_step_add_dep(&bin_dir_step, &out_dir_step, &arena);
    }

    AvenBuildStep compile_collatz_step;
    {
        char *src_path = aven_build_path(&arena, src_dir, "collatz.c", NULL);
        char *obj_path = aven_build_path(&arena, build_dir, "collatz.o", NULL);
        compile_collatz_step = cc_compile_obj(
            &copts,
            src_path,
            obj_path,
            &arena
        );
        aven_build_step_add_dep(&compile_collatz_step, &build_dir_step, &arena);
    }

    AvenBuildStep compile_main_step;
    {
        char *src_path = aven_build_path(&arena, src_dir, "main.c", NULL);
        char *obj_path = aven_build_path(&arena, build_dir, "main.o", NULL);
        compile_main_step = cc_compile_obj(&copts, src_path, obj_path, &arena);
        aven_build_step_add_dep(&compile_main_step, &build_dir_step, &arena);
    }

    AvenBuildStep link_exe_step;
    {
        AvenBuildStep *dep_objs[] = {
            &compile_collatz_step,
            &compile_main_step,
        };
        AvenBuildStepPtrSlice deps = {
            .ptr = dep_objs,
            .len = countof(dep_objs),
        };
        char *exe_path = aven_build_path(
            &arena,
            out_dir,
            bin_dir,
            exe_fname,
            NULL
        );
        link_exe_step = ld_link_exe(&ldopts, deps, exe_path, &arena);
        aven_build_step_add_dep(&link_exe_step, &bin_dir_step, &arena);
    }

    bool clean = aven_arg_get_bool(arg_slice, "-clean");
    bool watch = aven_arg_get_bool(arg_slice, "-watch");

    if (clean) {
        aven_build_step_clean(&link_exe_step);
    } else if (watch) {
        AvenWatchHandle src_handle = aven_watch_init(src_dir);
        for (;;) {
            error = aven_build_step_run(&link_exe_step, arena);
            if (error != 0) {
                printf("BUILD ERROR: %d\n", error);
            }
            aven_build_step_reset(&link_exe_step);
            aven_watch_check(src_handle, -1);
            while (aven_watch_check(src_handle, 100)) {}
            printf("\nRE-BUILDING:\n");
        }
        aven_watch_deinit(src_handle);
    } else {
        error = aven_build_step_run(&link_exe_step, arena);
        if (error != 0) {
            return error;
        }
    }

    free(mem);

    return 0;
}

