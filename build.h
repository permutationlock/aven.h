#ifndef BUILD_H
#define BUILD_H

#include <aven.h>
#include <aven/arg.h>
#include <aven/build.h>

AvenArg default_args[] = {
    {
        .name = "clean",
        .description = "Remove all build artifacts",
        .type = AVEN_ARG_TYPE_BOOL,
    },
    {
        .name = "-cc",
        .description = "C compiler exe",
        .type = AVEN_ARG_TYPE_STRING,
        .value = {
            .type = AVEN_ARG_TYPE_STRING,
#ifdef _WIN32
            .data = { .arg_str = "gcc.exe" },
#else
            .data = { .arg_str = "gcc" },
#endif
        },
    },
    {
        .name = "-cflags",
        .description = "C compiler common flags",
        .type = AVEN_ARG_TYPE_STRING,
        .value = {
            .type = AVEN_ARG_TYPE_STRING,
            .data = { .arg_str = "" },
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
            .data = { .arg_str = "" },
        },
    },
    {
        .name = "-ar",
        .description = "Archiver exe to create static libraries",
        .type = AVEN_ARG_TYPE_STRING,
        .value = {
            .type = AVEN_ARG_TYPE_STRING,
#ifdef _WIN32
            .data = { .arg_str = "ar.exe" },
#else
            .data = { .arg_str = "ar" },
#endif
        },
    },
    {
        .name = "-arflags",
        .description = "Archiver common flags",
        .type = AVEN_ARG_TYPE_STRING,
        .value = {
            .type = AVEN_ARG_TYPE_STRING,
            .data = { .arg_str = "-rcs" },
        },
    },
    {
        .name = "-exext",
        .description = "File extension for executables",
        .type = AVEN_ARG_TYPE_STRING,
        .value = {
            .type = AVEN_ARG_TYPE_STRING,
#ifdef _WIN32
            .data = { .arg_str = ".exe" },
#else
            .data = { .arg_str = "" },
#endif
        },
    },
    {
        .name = "-soext",
        .description = "File extension for shared libraries",
        .type = AVEN_ARG_TYPE_STRING,
        .value = {
            .type = AVEN_ARG_TYPE_STRING,
#ifdef _WIN32
            .data = { .arg_str = ".dll" },
#else
            .data = { .arg_str = ".so" },
#endif
        },
    },
    {
        .name = "-arext",
        .description = "File extension for static libraries",
        .type = AVEN_ARG_TYPE_STRING,
        .value = {
            .type = AVEN_ARG_TYPE_STRING,
            .data = { .arg_str = ".a" },
        },
    },
    {
        .name = "-cincflag",
        .description = "C compiler add to include path flag",
        .type = AVEN_ARG_TYPE_STRING,
        .value = {
            .type = AVEN_ARG_TYPE_STRING,
            .data = { .arg_str = "-I" },
        },
    },
    {
        .name = "-cdefflag",
        .description = "C compiler define macro flag",
        .type = AVEN_ARG_TYPE_STRING,
        .value = {
            .type = AVEN_ARG_TYPE_STRING,
            .data = { .arg_str = "-D" },
        },
    },
    {
        .name = "-cobjflag",
        .description = "C compiler compile object flag",
        .type = AVEN_ARG_TYPE_STRING,
        .value = {
            .type = AVEN_ARG_TYPE_STRING,
            .data = { .arg_str = "-c" },
        },
    },
    {
        .name = "-coutflag",
        .description = "C compiler output filename flag",
        .type = AVEN_ARG_TYPE_STRING,
        .value = {
            .type = AVEN_ARG_TYPE_STRING,
            .data = { .arg_str = "-o" },
        },
    },
    {
        .name = "-ldlibflag",
        .description = "Linker add library to link flag",
        .type = AVEN_ARG_TYPE_STRING,
        .value = {
            .type = AVEN_ARG_TYPE_STRING,
            .data = { .arg_str = "-l" },
        },
    },
    {
        .name = "-ldsoflag",
        .description = "Linker output shared library flag",
        .type = AVEN_ARG_TYPE_STRING,
        .value = {
            .type = AVEN_ARG_TYPE_STRING,
            .data = { .arg_str = "-shared" },
        },
    },
    {
        .name = "-ldoutflag",
        .description = "Linker output filename flag",
        .type = AVEN_ARG_TYPE_STRING,
        .value = {
            .type = AVEN_ARG_TYPE_STRING,
            .data = { .arg_str = "-o" },
        },
    },
    {
        .name = "-aroutflag",
        .description = "Archiver output filename flag",
        .type = AVEN_ARG_TYPE_STRING,
        .optional = true,
    },
};

typedef struct {
    char *compiler;
    char *objflag;
    char *outflag;
    char *incflag;
    char *defflag;
    AvenStrSlice flags;
} COpts;

typedef struct {
    char *linker;
    char *outflag;
    char *libflag;
    char *soflag;
    AvenStrSlice flags;
} LDOpts;

typedef struct {
    char *archiver;
    char *outflag;
    AvenStrSlice flags;
} AROpts;

typedef Slice(AvenBuildStep *) AvenBuildStepPtrSlice;

typedef struct {
    COpts cc;
    LDOpts ld;
    AROpts ar;
    char *exext;
    char *soext;
    char *arext;
    bool clean;
} DefaultOpts;

static DefaultOpts get_default_opts(AvenArgSlice arg_slice, AvenArena *arena) {
    DefaultOpts opts = { 0 };

    opts.clean = aven_arg_get_bool(arg_slice, "clean");
     
    opts.cc.compiler = aven_arg_get_str(arg_slice, "-cc");
    opts.cc.incflag = aven_arg_get_str(arg_slice, "-cincflag");
    opts.cc.defflag = aven_arg_get_str(arg_slice, "-cincflag");
    opts.cc.objflag = aven_arg_get_str(arg_slice, "-cobjflag");
    opts.cc.outflag = aven_arg_get_str(arg_slice, "-coutflag");
    {
        char *cflags_raw = aven_arg_get_str(arg_slice, "-cflags");
        AvenStrSliceResult result = aven_str_split(
            aven_str_from_cstr(cflags_raw),
            ' ',
            arena
        );
        assert(result.error == 0);

        opts.cc.flags = result.payload;
    }

    if (aven_arg_has_arg(arg_slice, "-ld")) {
        opts.ld.linker = aven_arg_get_str(arg_slice, "-ld");
        opts.ld.outflag = aven_arg_get_str(arg_slice, "-ldoutflag");
    } else {
        opts.ld.linker = opts.cc.compiler;
        opts.ld.outflag = opts.cc.outflag;
    }
    opts.ld.libflag = aven_arg_get_str(arg_slice, "-ldlibflag");
    opts.ld.soflag = aven_arg_get_str(arg_slice, "-ldsoflag");
    {
        char *ldflags_raw = aven_arg_get_str(arg_slice, "-ldflags");
        AvenStrSliceResult result = aven_str_split(
            aven_str_from_cstr(ldflags_raw),
            ' ',
            arena
        );
        assert(result.error == 0);

        opts.ld.flags = result.payload;
    }

    opts.ar.archiver = aven_arg_get_str(arg_slice, "-ar");
    if (aven_arg_has_arg(arg_slice, "-aroutflag")) {
        opts.ar.outflag = aven_arg_get_str(arg_slice, "-aroutflag");
    } else {
        opts.ar.outflag = "";
    }
    {
        char *arflags_raw = aven_arg_get_str(arg_slice, "-arflags");
        AvenStrSliceResult result = aven_str_split(
            aven_str_from_cstr(arflags_raw),
            ' ',
            arena
        );
        assert(result.error == 0);

        opts.ar.flags = result.payload;
    }

    opts.exext = aven_arg_get_str(arg_slice, "-exext");
    opts.soext = aven_arg_get_str(arg_slice, "-soext");
    opts.arext = aven_arg_get_str(arg_slice, "-arext");

    return opts;
}

typedef Slice(char *) CStrSlice;

static AvenBuildStep cc_compile_obj_ex(
    DefaultOpts *opts,
    CStrSlice includes,
    CStrSlice macros,
    char *src_path,
    char *target_path,
    AvenArena *arena
) {
    AvenBuildCmdSlice cmd_slice = {
        .len = 5 + opts->cc.flags.len + 2 * includes.len + 2 * macros.len
    };
    cmd_slice.ptr = aven_arena_create_array(char *, arena, cmd_slice.len);
    assert(cmd_slice.ptr != NULL);

    size_t i = 0;
    slice_get(cmd_slice, i) = opts->cc.compiler;
    i += 1;

    for (size_t j = 0; j < opts->cc.flags.len; j += 1) {
        slice_get(cmd_slice, i) = slice_get(opts->cc.flags, j).ptr;
        i += 1;
    }

    for (size_t j = 0; j < includes.len; j += 1) {
        slice_get(cmd_slice, i) = opts->cc.incflag;
        i += 1;
        slice_get(cmd_slice, i) = slice_get(includes, j);
        i += 1;
    }

    for (size_t j = 0; j < macros.len; j += 1) {
        slice_get(cmd_slice, i) = opts->cc.defflag;
        i += 1;
        slice_get(cmd_slice, i) = slice_get(macros, j);
        i += 1;
    }

    slice_get(cmd_slice, i) = opts->cc.objflag;
    i += 1;
    slice_get(cmd_slice, i) = opts->cc.outflag;
    i += 1;
    slice_get(cmd_slice, i) = target_path;
    i += 1;
    slice_get(cmd_slice, i) = src_path;
    i += 1;

    AvenBuildOptionalPath out_path = { .value = target_path, .valid = true };
    return aven_build_step_cmd_from_slice(out_path, cmd_slice);
}

static AvenBuildStep cc_compile_obj(
    DefaultOpts *opts,
    char *src_path,
    char *target_path,
    AvenArena *arena
) {
    return cc_compile_obj_ex(
        opts,
        (CStrSlice){ 0 },
        (CStrSlice){ 0 },
        src_path,
        target_path,
        arena
    );
}

static inline AvenBuildStep ld_link_bin(
    DefaultOpts *opts,
    CStrSlice linked_libs,
    AvenBuildStepPtrSlice obj_steps,
    char *target_path,
    bool shared_lib,
    AvenArena *arena
) {
    AvenStr target_str = aven_str_from_cstr(target_path);
    AvenStr ext = { 0 };
    if (shared_lib) {
        ext = aven_str_from_cstr(opts->soext);
    } else {
        ext = aven_str_from_cstr(opts->exext);
    }
    if (ext.len > 0) {
        AvenStrResult result = aven_str_concat(target_str, ext, arena);
        assert(result.error == 0);
        target_str = result.payload;
    }

    AvenBuildCmdSlice cmd_slice = { 0 };
    cmd_slice.len = 3 +
        opts->ld.flags.len +
        obj_steps.len +
        2 * linked_libs.len;
    if (shared_lib) {
        cmd_slice.len += 1;
    }
    cmd_slice.ptr = aven_arena_create_array(char *, arena, cmd_slice.len);
    assert(cmd_slice.ptr != NULL);

    size_t i = 0;
    slice_get(cmd_slice, i) = opts->ld.linker;
    i += 1;

    for (size_t j = 0; j < opts->ld.flags.len; j += 1) {
        slice_get(cmd_slice, i) = slice_get(opts->ld.flags, j).ptr;
        i += 1;
    }

    if (shared_lib) {
        slice_get(cmd_slice, i) = opts->ld.soflag;
        i += 1;
    }

    for (size_t j = 0; j < linked_libs.len; j += 1) {
        slice_get(cmd_slice, i) = opts->ld.libflag;
        i += 1;
        slice_get(cmd_slice, i) = slice_get(linked_libs, j);
        i += 1;
    }

    slice_get(cmd_slice, i) = opts->ld.outflag;
    i += 1;
    slice_get(cmd_slice, i) = target_str.ptr;
    i += 1;

    for (size_t j = 0; j < obj_steps.len; j += 1) {
        AvenBuildStep *obj_step = slice_get(obj_steps, j);
        assert(obj_step->out_path.valid);
        slice_get(cmd_slice, i) = obj_step->out_path.value;
        i += 1;
    }

    AvenBuildOptionalPath out_path = { .value = target_str.ptr, .valid = true };
    AvenBuildStep link_step = aven_build_step_cmd_from_slice(
        out_path,
        cmd_slice
    );

    for (size_t j = 0; j < obj_steps.len; j += 1) {
        aven_build_step_add_dep(&link_step, slice_get(obj_steps, j), arena);
    }

    return link_step;
}

static inline AvenBuildStep ld_link_exe_ex(
    DefaultOpts *opts,
    CStrSlice linked_libs,
    AvenBuildStepPtrSlice obj_steps,
    char *target_path,
    AvenArena *arena
) {
    return ld_link_bin(opts, linked_libs, obj_steps, target_path, false, arena);
}

static inline AvenBuildStep ld_link_so_ex(
    DefaultOpts *opts,
    CStrSlice linked_libs,
    AvenBuildStepPtrSlice obj_steps,
    char *target_path,
    AvenArena *arena
) {
    return ld_link_bin(opts, linked_libs, obj_steps, target_path, true, arena);
}

static inline AvenBuildStep ld_link_exe(
    DefaultOpts *opts,
    AvenBuildStepPtrSlice obj_steps,
    char *target_path,
    AvenArena *arena
) {
    return ld_link_exe_ex(
        opts,
        (CStrSlice){ 0 },
        obj_steps,
        target_path,
        arena
    );
}

static inline AvenBuildStep ld_link_so(
    DefaultOpts *opts,
    AvenBuildStepPtrSlice obj_steps,
    char *target_path,
    AvenArena *arena
) {
    return ld_link_so_ex(
        opts,
        (CStrSlice){ 0 },
        obj_steps,
        target_path,
        arena
    );
}

static inline AvenBuildStep ar_create_lib(
    DefaultOpts *opts,
    AvenBuildStepPtrSlice obj_steps,
    char *target_path,
    AvenArena *arena
) {
    AvenStr target_str = aven_str_from_cstr(target_path);
    AvenStr ext = aven_str_from_cstr(opts->arext);
    if (ext.len > 0) {
        AvenStrResult result = aven_str_concat(target_str, ext, arena);
        assert(result.error == 0);
        target_str = result.payload;
    }

    AvenBuildCmdSlice cmd_slice = { 0 };
    cmd_slice.len = 2 + opts->ar.flags.len + obj_steps.len;
    if (opts->ar.outflag[0] != 0) {
        cmd_slice.len += 1;
    }
    cmd_slice.ptr = aven_arena_create_array(char *, arena, cmd_slice.len);
    assert(cmd_slice.ptr != NULL);

    size_t i = 0;
    slice_get(cmd_slice, i) = opts->ar.archiver;
    i += 1;

    for (size_t j = 0; j < opts->ar.flags.len; j += 1) {
        slice_get(cmd_slice, i) = slice_get(opts->ar.flags, j).ptr;
        i += 1;
    }

    if (opts->ar.outflag[0] != 0) {
        slice_get(cmd_slice, i) = opts->ar.outflag;
        i += 1;
    }

    slice_get(cmd_slice, i) = target_str.ptr;
    i += 1;

    for (size_t j = 0; j < obj_steps.len; j += 1) {
        AvenBuildStep *obj_step = slice_get(obj_steps, j);
        assert(obj_step->out_path.valid);
        slice_get(cmd_slice, i) = obj_step->out_path.value;
        i += 1;
    }

    AvenBuildOptionalPath out_path = { .value = target_str.ptr, .valid = true };
    AvenBuildStep ar_step = aven_build_step_cmd_from_slice(
        out_path,
        cmd_slice
    );

    for (size_t j = 0; j < obj_steps.len; j += 1) {
        aven_build_step_add_dep(&ar_step, slice_get(obj_steps, j), arena);
    }

    return ar_step;
}

#endif // BUILD_H
