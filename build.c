#define _POSIX_C_SOURCE 200809L

#include <stdlib.h>

#include <aven.h>
#include <aven/arg.h>
#include <aven/build.h>

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
    {
        .name = "-clean",
        .description = "Remove all build artifacts",
        .type = AVEN_ARG_TYPE_BOOL,
    },
};

typedef struct {
    AvenString compiler;
    AvenStringSlice flags;
    AvenString objflag;
    AvenString outflag;
} COpts;

AvenBuildStep cc_compile_object(
    COpts *copts,
    AvenString src_path,
    AvenString target_path,
    AvenArena *arena
) {
    AvenStringSlice cmd_slice = { .len = copts->flags.len + 5 };
    cmd_slice.ptr = aven_arena_create_array(AvenString, arena, cmd_slice.len);
    assert(cmd_slice.ptr != NULL);

    size_t i = 0;
    slice_get(cmd_slice, i) = copts->compiler;
    i += 1;

    for (size_t j = 0; j < copts->flags.len; j += 1) {    
        slice_get(cmd_slice, i) = slice_get(copts->flags, j);
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

    return aven_build_step_cmd(
        cmd_slice,
        (AvenBuildOptionalPath){ .value = target_path, .valid = true }
    );
}

typedef struct {
    AvenString linker;
    AvenStringSlice flags;
    AvenString outflag;
} LDOpts;

typedef Slice(AvenBuildStep *) AvenBuildStepPtrSlice;

AvenBuildStep ld_link_exe(
    LDOpts *ldopts,
    AvenBuildStepPtrSlice obj_steps,
    AvenString target_path,
    AvenArena *arena
) {
    AvenStringSlice cmd_slice = {
        .len = ldopts->flags.len + obj_steps.len + 3
    };
    cmd_slice.ptr = aven_arena_create_array(AvenString, arena, cmd_slice.len);
    assert(cmd_slice.ptr != NULL);

    size_t i = 0;
    slice_get(cmd_slice, i) = ldopts->linker;
    i += 1;

    for (size_t j = 0; j < ldopts->flags.len; j += 1) {
        slice_get(cmd_slice, i) = slice_get(ldopts->flags, j);
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

    AvenBuildStep link_step = aven_build_step_cmd(
        cmd_slice,
        (AvenBuildOptionalPath){ .value = target_path, .valid = true }
    );

    for (size_t j = 0; j < obj_steps.len; j += 1) {
        AvenBuildStep *obj_step = slice_get(obj_steps, j);
        int error = aven_build_step_add_dep(&link_step, obj_step, arena);
        assert(error == 0);
    }

    return link_step;
}

#define ARENA_SIZE (4096 * 2000)

int main(int argc, char **argv) {
    void *mem = malloc(ARENA_SIZE);
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
    copts.compiler = aven_string_from_cstr(
        aven_arg_get_string(arg_slice, "-cc")
    );
    copts.objflag = aven_string_from_cstr(
        aven_arg_get_string(arg_slice, "-cobjflag")
    );
    copts.outflag = aven_string_from_cstr(
        aven_arg_get_string(arg_slice, "-coutflag")
    );
    {
        AvenString cflags_raw = aven_string_from_cstr(
            aven_arg_get_string(arg_slice, "-cflags")
        );
        AvenStringSliceResult result = aven_string_split(
            cflags_raw,
            ' ',
            &arena
        );
        assert(result.error == 0);

        copts.flags = result.payload;
    }

    LDOpts ldopts = { 0 };
    if (aven_arg_has_arg(arg_slice, "-ld")) {
        ldopts.linker = aven_string_from_cstr(
            aven_arg_get_string(arg_slice, "-ld")
        );
        ldopts.outflag = aven_string_from_cstr(
            aven_arg_get_string(arg_slice, "-ldoutflag")
        );
    } else {
        ldopts.linker = copts.compiler;
        ldopts.outflag = copts.outflag;
    }
    {
        AvenString ldflags_raw = aven_string_from_cstr(
            aven_arg_get_string(arg_slice, "-ldflags")
        );
        AvenStringSliceResult result = aven_string_split(
            ldflags_raw,
            ' ',
            &arena
        );
        assert(result.error == 0);

        ldopts.flags = result.payload;
    }

    AvenString build_dir_str = aven_string_from_cstr("build_temp");
    AvenBuildStep build_dir_step = aven_build_step_mkdir(build_dir_str);

    AvenString out_dir_str = aven_string_from_cstr("build_out");
    AvenBuildStep out_dir_step = aven_build_step_mkdir(out_dir_str);

    char *bin_dir_parts[] = { out_dir_str.ptr, "bin" };
    AvenString bin_dir_str = aven_build_path(
        bin_dir_parts,
        countof(bin_dir_parts),
        &arena 
    );
    AvenBuildStep bin_dir_step = aven_build_step_mkdir(bin_dir_str);
    aven_build_step_add_dep(&bin_dir_step, &out_dir_step, &arena);

    char src_dir[] = "src";

    AvenBuildStep compile_collatz_step;
    {
        char *src_path_parts[] = { src_dir, "collatz.c" };
        AvenString src_path = aven_build_path(
            src_path_parts,
            countof(src_path_parts),
            &arena
        );
        char *obj_path_parts[] = { build_dir_str.ptr, "collatz.o" };
        AvenString obj_path = aven_build_path(
            obj_path_parts,
            countof(obj_path_parts),
            &arena
        );

        compile_collatz_step = cc_compile_object(
            &copts,
            src_path,
            obj_path,
            &arena
        );
        error = aven_build_step_add_dep(
            &compile_collatz_step,
            &build_dir_step,
            &arena
        );
        assert(error == 0);
    }

    AvenBuildStep compile_main_step;
    {
        char *src_path_parts[] = { src_dir, "main.c" };
        AvenString src_path = aven_build_path(
            src_path_parts,
            countof(src_path_parts),
            &arena
        );
        char *obj_path_parts[] = { build_dir_str.ptr, "main.o" };
        AvenString obj_path = aven_build_path(
            obj_path_parts,
            countof(obj_path_parts),
            &arena
        );

        compile_main_step = cc_compile_object(
            &copts,
            src_path,
            obj_path,
            &arena
        );
        error = aven_build_step_add_dep(
            &compile_main_step,
            &build_dir_step,
            &arena
        );
        assert(error == 0);
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

        char *exe_path_parts[] = { bin_dir_str.ptr, "print_collatz" };
        AvenString exe_path = aven_build_path(
            exe_path_parts,
            countof(exe_path_parts),
            &arena
        );

        link_exe_step = ld_link_exe(
            &ldopts,
            deps,
            exe_path,
            &arena
        );
        error = aven_build_step_add_dep(
            &link_exe_step,
            &bin_dir_step,
            &arena
        );
        assert(error == 0);
    }

    bool clean = aven_arg_get_bool(arg_slice, "-clean");

    aven_build_step_clean(&link_exe_step);
    if (!clean) {
        error = aven_build_step_run(&link_exe_step, arena);
        if (error != 0) {
            return error;
        }
    }

    free(mem);

    return 0;
}
