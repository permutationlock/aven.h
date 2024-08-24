#ifndef LIBAVEN_BUILD_H
#define LIBAVEN_BUILD_H

#include "include/aven.h"
#include "include/aven/arena.h"
#include "include/aven/arg.h"
#include "include/aven/build.h"
#include "include/aven/build/common.h"
#include "include/aven/path.h"
#include "include/aven/str.h"

static AvenStr libaven_build_include_path(AvenStr root_path, AvenArena *arena) {
    return aven_path(arena, root_path.ptr, "include", NULL);
}

static AvenBuildStep libaven_build_step(
    AvenBuildCommonOpts *opts,
    AvenStr root_path,
    AvenBuildStep *work_dir_step,
    AvenBuildStep *out_dir_step,
    AvenArena *arena
) {
    char *src_files[] = {
        "arena.c",
        "arg.c",
        "build.c",
        "build_common.c",
        "dl.c",
        "path.c",
        "watch.c",
    };
    AvenBuildStep *obj_ref_data[countof(src_files)];
    AvenBuildStepPtrSlice obj_refs = {
        .ptr = obj_ref_data,
        .len = countof(obj_ref_data),
    };

    AvenBuildStepSlice obj_steps = { .len = countof(src_files) };
    obj_steps.ptr = aven_arena_create_array(
        AvenBuildStep,
        arena,
        countof(src_files)
    );

    AvenStr include_paths[] = { libaven_build_include_path(root_path, arena) };
    AvenStrSlice includes = {
        .ptr = include_paths,
        .len = countof(include_paths),
    };
    AvenStrSlice macros = { 0 };

    for (size_t i = 0; i < countof(src_files); i += 1) {
        AvenBuildStep *obj_step = &slice_get(obj_steps, i);
        *obj_step = aven_build_common_step_cc_ex(
            opts,
            includes,
            macros,
            aven_path(arena, root_path.ptr, "src", src_files[i], NULL),
            work_dir_step,
            arena
        );
        slice_get(obj_refs, i) = obj_step;
    }

    return aven_build_common_step_ar(
        opts,
        obj_refs,
        out_dir_step,
        aven_str("libaven"),
        arena
    );
}

#endif // LIBAVEN_BUILD_H
