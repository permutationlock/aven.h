#ifndef LIBAVEN_BUILD_H
#define LIBAVEN_BUILD_H

static inline AvenStr libaven_build_include_path(
    AvenStr root_path,
    AvenArena *arena
) {
    return aven_path(arena, root_path.ptr, "include", NULL);
}

static inline AvenBuildStep libaven_build_step(
    AvenBuildCommonOpts *opts,
    AvenStr root_path,
    AvenBuildStep *out_dir_step,
    AvenArena *arena
) {
    AvenStr include_paths[] = { libaven_build_include_path(root_path, arena) };
    AvenStrSlice includes = {
        .ptr = include_paths,
        .len = countof(include_paths),
    };
    AvenStrSlice macros = { 0 };

    return aven_build_common_step_cc_ex(
        opts,
        includes,
        macros,
        aven_path(arena, root_path.ptr, "src", "aven.c", NULL),
        out_dir_step,
        arena
    );
}

#endif // LIBAVEN_BUILD_H
