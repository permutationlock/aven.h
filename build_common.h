#ifndef BUILD_H
#define BUILD_H

#include "aven/include/aven.h"
#include "aven/include/aven/arena.h"
#include "aven/include/aven/arg.h"
#include "aven/include/aven/build.h"
#include "aven/include/aven/path.h"
#include "aven/include/aven/str.h"

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
        .description = "C compiler flag to add an include path",
        .type = AVEN_ARG_TYPE_STRING,
        .value = {
            .type = AVEN_ARG_TYPE_STRING,
            .data = { .arg_str = "-I" },
        },
    },
    {
        .name = "-cdefflag",
        .description = "C compiler flag to define macro",
        .type = AVEN_ARG_TYPE_STRING,
        .value = {
            .type = AVEN_ARG_TYPE_STRING,
            .data = { .arg_str = "-D" },
        },
    },
    {
        .name = "-cobjflag",
        .description = "C compiler flag to compile object",
        .type = AVEN_ARG_TYPE_STRING,
        .value = {
            .type = AVEN_ARG_TYPE_STRING,
            .data = { .arg_str = "-c" },
        },
    },
    {
        .name = "-coutflag",
        .description = "C compiler flag to specify output filename",
        .type = AVEN_ARG_TYPE_STRING,
        .value = {
            .type = AVEN_ARG_TYPE_STRING,
            .data = { .arg_str = "-o" },
        },
    },
    {
        .name = "-ldlibflag",
        .description = "Linker flag to link library",
        .type = AVEN_ARG_TYPE_STRING,
        .value = {
            .type = AVEN_ARG_TYPE_STRING,
            .data = { .arg_str = "-l" },
        },
    },
    {
        .name = "-ldsoflag",
        .description = "Linker flag to emit shared library",
        .type = AVEN_ARG_TYPE_STRING,
        .value = {
            .type = AVEN_ARG_TYPE_STRING,
            .data = { .arg_str = "-shared" },
        },
    },
    {
        .name = "-ldoutflag",
        .description = "Linker flag to specify output filename",
        .type = AVEN_ARG_TYPE_STRING,
        .value = {
            .type = AVEN_ARG_TYPE_STRING,
            .data = { .arg_str = "-o" },
        },
    },
    {
        .name = "-aroutflag",
        .description = "Archiver flag to specify output filename",
        .type = AVEN_ARG_TYPE_STRING,
        .optional = true,
    },
};

typedef struct {
    AvenStr compiler;
    AvenStr objflag;
    AvenStr outflag;
    AvenStr incflag;
    AvenStr defflag;
    AvenStrSlice flags;
} COpts;

typedef struct {
    AvenStr linker;
    AvenStr outflag;
    AvenStr libflag;
    AvenStr soflag;
    AvenStrSlice flags;
} LDOpts;

typedef struct {
    AvenStr archiver;
    AvenStr outflag;
    AvenStrSlice flags;
} AROpts;

typedef Slice(AvenBuildStep *) AvenBuildStepPtrSlice;

typedef struct {
    COpts cc;
    LDOpts ld;
    AROpts ar;
    AvenStr exext;
    AvenStr soext;
    AvenStr arext;
    bool clean;
} DefaultOpts;

static DefaultOpts get_default_opts(AvenArgSlice arg_slice, AvenArena *arena) {
    DefaultOpts opts = { 0 };

    opts.clean = aven_arg_get_bool(arg_slice, "clean");
     
    opts.cc.compiler = aven_str_cstr(aven_arg_get_str(arg_slice, "-cc"));
    opts.cc.incflag = aven_str_cstr(aven_arg_get_str(arg_slice, "-cincflag"));
    opts.cc.defflag = aven_str_cstr(aven_arg_get_str(arg_slice, "-cdefflag"));
    opts.cc.objflag = aven_str_cstr(aven_arg_get_str(arg_slice, "-cobjflag"));
    opts.cc.outflag = aven_str_cstr(aven_arg_get_str(arg_slice, "-coutflag"));
    opts.cc.flags = aven_str_split(
        aven_str_cstr(aven_arg_get_str(arg_slice, "-cflags")),
        ' ',
        arena
    );

    if (aven_arg_has_arg(arg_slice, "-ld")) {
        opts.ld.linker = aven_str_cstr(aven_arg_get_str(arg_slice, "-ld"));
        opts.ld.outflag = aven_str_cstr(
            aven_arg_get_str(arg_slice, "-ldoutflag")
        );
    } else {
        opts.ld.linker = opts.cc.compiler;
        opts.ld.outflag = opts.cc.outflag;
    }
    opts.ld.libflag = aven_str_cstr(aven_arg_get_str(arg_slice, "-ldlibflag"));
    opts.ld.soflag = aven_str_cstr(aven_arg_get_str(arg_slice, "-ldsoflag"));
    opts.ld.flags = aven_str_split(
        aven_str_cstr(aven_arg_get_str(arg_slice, "-ldflags")),
        ' ',
        arena
    );

    opts.ar.archiver = aven_str_cstr(aven_arg_get_str(arg_slice, "-ar"));
    if (aven_arg_has_arg(arg_slice, "-aroutflag")) {
        opts.ar.outflag = aven_str_cstr(
            aven_arg_get_str(arg_slice, "-aroutflag")
        );
    } else {
        opts.ar.outflag = aven_str("");
    }
    opts.ar.flags = aven_str_split(
        aven_str_cstr(aven_arg_get_str(arg_slice, "-arflags")),
        ' ',
        arena
    );

    opts.exext = aven_str_cstr(aven_arg_get_str(arg_slice, "-exext"));
    opts.soext = aven_str_cstr(aven_arg_get_str(arg_slice, "-soext"));
    opts.arext = aven_str_cstr(aven_arg_get_str(arg_slice, "-arext"));

    return opts;
}

static AvenBuildStep cc_compile_obj_ex(
    DefaultOpts *opts,
    AvenStrSlice includes,
    AvenStrSlice macros,
    AvenStr src_path,
    AvenStr target_path,
    AvenArena *arena
) {
    assert(target_path.len > 3);
    AvenStr target_ext_str = {
        .ptr = target_path.ptr + (target_path.len - 2),
        .len = 2,
    };
    if (!aven_str_compare(target_ext_str, aven_str(".o"))) {
        AvenStr src_fname = aven_path_fname(src_path, arena);
        AvenStrSlice ext_split = aven_str_split(
            src_fname,
            '.',
            arena
        );
        AvenStr src_fname_body = slice_get(ext_split, 0);
        AvenStr fname = aven_str_concat(
            src_fname_body,
            aven_str(".o"),
            arena
        );

        target_path = aven_path(arena, target_path.ptr, fname.ptr, NULL);
    }

    AvenStrSlice cmd_slice = {
        .len = 5 + opts->cc.flags.len + 2 * includes.len + 2 * macros.len
    };
    cmd_slice.ptr = aven_arena_create_array(AvenStr, arena, cmd_slice.len);

    size_t i = 0;
    slice_get(cmd_slice, i) = opts->cc.compiler;
    i += 1;

    for (size_t j = 0; j < opts->cc.flags.len; j += 1) {
        slice_get(cmd_slice, i) = slice_get(opts->cc.flags, j);
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
    return aven_build_step_cmd(out_path, cmd_slice);
}

static AvenBuildStep cc_compile_obj(
    DefaultOpts *opts,
    AvenStr src_path,
    AvenStr target_path,
    AvenArena *arena
) {
    return cc_compile_obj_ex(
        opts,
        (AvenStrSlice){ 0 },
        (AvenStrSlice){ 0 },
        src_path,
        target_path,
        arena
    );
}

static inline AvenBuildStep ld_link_bin(
    DefaultOpts *opts,
    AvenStrSlice linked_libs,
    AvenBuildStepPtrSlice obj_steps,
    AvenStr target_path,
    bool shared_lib,
    AvenArena *arena
) {
    AvenStr ext = { 0 };
    if (shared_lib) {
        ext = opts->soext;
    } else {
        ext = opts->exext;
    }
    if (ext.len > 0) {
        target_path = aven_str_concat(target_path, ext, arena);
    }

    AvenStrSlice cmd_slice = { 0 };
    cmd_slice.len = 3 +
        opts->ld.flags.len +
        obj_steps.len +
        2 * linked_libs.len;
    if (shared_lib) {
        cmd_slice.len += 1;
    }
    cmd_slice.ptr = aven_arena_create_array(AvenStr, arena, cmd_slice.len);

    size_t i = 0;
    slice_get(cmd_slice, i) = opts->ld.linker;
    i += 1;

    for (size_t j = 0; j < opts->ld.flags.len; j += 1) {
        slice_get(cmd_slice, i) = slice_get(opts->ld.flags, j);
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
    slice_get(cmd_slice, i) = target_path;
    i += 1;

    for (size_t j = 0; j < obj_steps.len; j += 1) {
        AvenBuildStep *obj_step = slice_get(obj_steps, j);
        assert(obj_step->out_path.valid);
        slice_get(cmd_slice, i) = obj_step->out_path.value;
        i += 1;
    }

    AvenBuildOptionalPath out_path = { .value = target_path, .valid = true };
    AvenBuildStep link_step = aven_build_step_cmd(
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
    AvenStrSlice linked_libs,
    AvenBuildStepPtrSlice obj_steps,
    AvenStr target_path,
    AvenArena *arena
) {
    return ld_link_bin(opts, linked_libs, obj_steps, target_path, false, arena);
}

static inline AvenBuildStep ld_link_so_ex(
    DefaultOpts *opts,
    AvenStrSlice linked_libs,
    AvenBuildStepPtrSlice obj_steps,
    AvenStr target_path,
    AvenArena *arena
) {
    return ld_link_bin(opts, linked_libs, obj_steps, target_path, true, arena);
}

static inline AvenBuildStep ld_link_exe(
    DefaultOpts *opts,
    AvenBuildStepPtrSlice obj_steps,
    AvenStr target_path,
    AvenArena *arena
) {
    return ld_link_exe_ex(
        opts,
        (AvenStrSlice){ 0 },
        obj_steps,
        target_path,
        arena
    );
}

static inline AvenBuildStep ld_link_so(
    DefaultOpts *opts,
    AvenBuildStepPtrSlice obj_steps,
    AvenStr target_path,
    AvenArena *arena
) {
    return ld_link_so_ex(
        opts,
        (AvenStrSlice){ 0 },
        obj_steps,
        target_path,
        arena
    );
}

static inline AvenBuildStep ar_create_lib(
    DefaultOpts *opts,
    AvenBuildStepPtrSlice obj_steps,
    AvenStr target_path,
    AvenArena *arena
) {
    AvenStr ext = opts->arext;
    if (ext.len > 0) {
        target_path = aven_str_concat(target_path, ext, arena);
    }

    AvenStrSlice cmd_slice = { 0 };
    cmd_slice.len = 2 + opts->ar.flags.len + obj_steps.len;
    if (opts->ar.outflag.len != 0) {
        cmd_slice.len += 1;
    }
    cmd_slice.ptr = aven_arena_create_array(AvenStr, arena, cmd_slice.len);

    size_t i = 0;
    slice_get(cmd_slice, i) = opts->ar.archiver;
    i += 1;

    for (size_t j = 0; j < opts->ar.flags.len; j += 1) {
        slice_get(cmd_slice, i) = slice_get(opts->ar.flags, j);
        i += 1;
    }

    if (opts->ar.outflag.len != 0) {
        slice_get(cmd_slice, i) = opts->ar.outflag;
        i += 1;
    }

    slice_get(cmd_slice, i) = target_path;
    i += 1;

    for (size_t j = 0; j < obj_steps.len; j += 1) {
        AvenBuildStep *obj_step = slice_get(obj_steps, j);
        assert(obj_step->out_path.valid);
        slice_get(cmd_slice, i) = obj_step->out_path.value;
        i += 1;
    }

    AvenBuildOptionalPath out_path = { .value = target_path, .valid = true };
    AvenBuildStep ar_step = aven_build_step_cmd(
        out_path,
        cmd_slice
    );

    for (size_t j = 0; j < obj_steps.len; j += 1) {
        aven_build_step_add_dep(&ar_step, slice_get(obj_steps, j), arena);
    }

    return ar_step;
}

char *aven_src_files[] = {
    "arena.c",
    "arg.c",
    "build.c",
    "dl.c",
    "path.c",
    "watch.c",
};

typedef Slice(AvenBuildStep) AvenBuildStepSlice;

static inline AvenBuildStep build_aven_step(
    DefaultOpts *opts,
    AvenStr aven_inc_path,
    AvenStr aven_src_path,
    AvenBuildStep *work_path_step,
    AvenBuildStep *out_path_step,
    AvenArena *arena
) {
    assert(work_path_step->out_path.valid);
    AvenStr work_path = work_path_step->out_path.value;

    assert(out_path_step->out_path.valid);
    AvenStr out_path = out_path_step->out_path.value;

    AvenBuildStepSlice obj_steps = { .len = countof(aven_src_files) };
    obj_steps.ptr = aven_arena_create_array(
        AvenBuildStep,
        arena,
        countof(aven_src_files)
    );

    AvenStr include_array[] = { aven_inc_path };
    AvenStrSlice includes = {
        .ptr = include_array,
        .len = countof(include_array),
    };

    AvenStrSlice macros = { 0 };

    for (size_t i = 0; i < countof(aven_src_files); i += 1) {
        AvenStr src_path = aven_path(
            arena,
            aven_src_path.ptr,
            aven_src_files[i],
            NULL
        );
        AvenBuildStep *obj_step = &slice_get(obj_steps, i);
        *obj_step = cc_compile_obj_ex(
            opts,
            includes,
            macros,
            src_path,
            work_path,
            arena
        );
        aven_build_step_add_dep(obj_step, work_path_step, arena);
    }

    AvenBuildStepPtrSlice obj_refs = { .len = obj_steps.len };
    obj_refs.ptr = aven_arena_create_array(
        AvenBuildStep *,
        arena,
        obj_refs.len
    );
    
    for (size_t i = 0; i < obj_steps.len; i += 1) {
        slice_get(obj_refs, i) = &slice_get(obj_steps, i);
    }

    AvenStr lib_path = aven_path(arena, out_path.ptr, "libaven", NULL);
    AvenBuildStep lib_step = ar_create_lib(opts, obj_refs, lib_path, arena);
    aven_build_step_add_dep(&lib_step, out_path_step, arena);
    return lib_step;
}

#endif // BUILD_H
