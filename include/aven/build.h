#ifndef AVEN_BUILD_H
#define AVEN_BUILD_H

#include "../aven.h"
#include "arena.h"
#include "fs.h"
#include "str.h"

#define AVEN_BUILD_MAX_ARGS 32

#ifdef _WIN32
    typedef void *AvenBuildPID;
#else
    typedef int AvenBuildPID;
#endif

typedef Result(AvenBuildPID) AvenBuildPIDResult;
typedef enum {
    AVEN_BUILD_CMD_ERROR_NONE = 0,
    AVEN_BUILD_CMD_ERROR_FORK,
} AvenBuildRunCmdError;

AVEN_FN AvenBuildPIDResult aven_build_cmd_run(
    AvenStrSlice cmd,
    AvenArena arena
);

typedef enum {
    AVEN_BUILD_WAIT_ERROR_NONE = 0,
    AVEN_BUILD_WAIT_ERROR_WAIT,
    AVEN_BUILD_WAIT_ERROR_GETCODE,
    AVEN_BUILD_WAIT_ERROR_PROCESS,
    AVEN_BUILD_WAIT_ERROR_SIGNAL,
} AvenBuildWaitError;

AVEN_FN int aven_build_cmd_wait(AvenBuildPID pid);

typedef enum {
    AVEN_BUILD_STEP_STATE_NONE = 0,
    AVEN_BUILD_STEP_STATE_RUNNING,
    AVEN_BUILD_STEP_STATE_DONE,
} AvenBuildStepState;

typedef enum {
    AVEN_BUILD_STEP_TYPE_ROOT = 0,
    AVEN_BUILD_STEP_TYPE_PATH,
    AVEN_BUILD_STEP_TYPE_CMD,
    AVEN_BUILD_STEP_TYPE_RM,
    AVEN_BUILD_STEP_TYPE_RMDIR,
    AVEN_BUILD_STEP_TYPE_MKDIR,
    AVEN_BUILD_STEP_TYPE_TRUNC,
    AVEN_BUILD_STEP_TYPE_COPY,
} AvenBuildStepType;

typedef union {
    AvenStrSlice cmd;
    AvenStr rm;
    AvenStr rmdir;
    AvenStr copy;
} AvenBuildStepData;

typedef Optional(AvenStr) AvenBuildOptionalPath;
typedef struct AvenBuildStepNode AvenBuildStepNode;

typedef struct AvenBuildStep {
    AvenBuildStepNode *dep;

    AvenBuildStepState state;
    AvenBuildPID pid;

    AvenBuildStepType type;
    AvenBuildStepData data;

    AvenBuildOptionalPath out_path;
} AvenBuildStep;

struct AvenBuildStepNode {
    AvenBuildStepNode *next;
    AvenBuildStep *step;
};

typedef Slice(AvenBuildStep) AvenBuildStepSlice;
typedef Slice(AvenBuildStep *) AvenBuildStepPtrSlice;

static inline AvenBuildStep aven_build_step_cmd(
    AvenBuildOptionalPath out_path,
    AvenStrSlice cmd_slice
) {
    return (AvenBuildStep){
        .type = AVEN_BUILD_STEP_TYPE_CMD,
        .data = { .cmd = cmd_slice },
        .out_path = out_path,
    };
}

static inline AvenBuildStep aven_build_step_root(void) {
    return (AvenBuildStep){ .type = AVEN_BUILD_STEP_TYPE_ROOT };
}

static inline AvenBuildStep aven_build_step_path(AvenStr path) {
    return (AvenBuildStep){
        .type = AVEN_BUILD_STEP_TYPE_PATH,
        .out_path = { .valid = true, .value = path },
    };
}

static inline AvenBuildStep aven_build_step_mkdir(AvenStr dir_path) {
    return (AvenBuildStep){
        .type = AVEN_BUILD_STEP_TYPE_MKDIR,
        .out_path = { .valid = true, .value = dir_path },
    };
}

static inline AvenBuildStep aven_build_step_rm(AvenStr file_path) {
    return (AvenBuildStep){
        .type = AVEN_BUILD_STEP_TYPE_RM,
        .data = { .rm = file_path },
    };
}

static inline  AvenBuildStep aven_build_step_rmdir(AvenStr dir_path) {
    return (AvenBuildStep){
        .type = AVEN_BUILD_STEP_TYPE_RMDIR,
        .data = { .rmdir = dir_path },
    };
}

static inline AvenBuildStep aven_build_step_trunc(AvenStr file_path) {
    return (AvenBuildStep){
        .type = AVEN_BUILD_STEP_TYPE_TRUNC,
        .out_path = { .valid = true, .value = file_path },
    };
}

static inline AvenBuildStep aven_build_step_copy(
    AvenStr in_file_path,
    AvenStr out_file_path
) {
    return (AvenBuildStep){
        .type = AVEN_BUILD_STEP_TYPE_COPY,
        .data = { .copy = in_file_path },
        .out_path = { .valid = true, .value = out_file_path },
    };
}

static inline void aven_build_step_add_dep(
    AvenBuildStep *step,
    AvenBuildStep *dep,
    AvenArena *arena
) {
    AvenBuildStepNode *node = aven_arena_create(
        AvenBuildStepNode,
        arena
    );
    *node = (AvenBuildStepNode){
        .next = step->dep,
        .step = dep,
    };
    step->dep = node;
}

typedef enum {
    AVEN_BUILD_STEP_RUN_ERROR_NONE = 0,
    AVEN_BUILD_STEP_RUN_ERROR_DEPRUN,
    AVEN_BUILD_STEP_RUN_ERROR_DEPWAIT,
    AVEN_BUILD_STEP_RUN_ERROR_CMD,
    AVEN_BUILD_STEP_RUN_ERROR_RM,
    AVEN_BUILD_STEP_RUN_ERROR_RMDIR,
    AVEN_BUILD_STEP_RUN_ERROR_MKDIR,
    AVEN_BUILD_STEP_RUN_ERROR_TRUNC,
    AVEN_BUILD_STEP_RUN_ERROR_COPY,
    AVEN_BUILD_STEP_RUN_ERROR_OUTPATH,
    AVEN_BUILD_STEP_RUN_ERROR_BADTYPE,
} AvenBuildStepRunError;

AVEN_FN int aven_build_step_run(AvenBuildStep *step, AvenArena arena);
AVEN_FN void aven_build_step_clean(AvenBuildStep *step);
AVEN_FN void aven_build_step_reset(AvenBuildStep *step);

#ifdef AVEN_IMPLEMENTATION

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>

#ifdef _WIN32
    typedef struct {
        uint32_t len;
        void *security_descriptor;
        int inherit_handle;
    } WinSecurityAttr;

    #define WIN_STARTF_USESTDHANDLES 0x00000100

    typedef struct {
        uint32_t cb;
        char *lpReserved;
        char *lpDesktop;
        char *lpTitle;
        uint32_t dwX;
        uint32_t dwY;
        uint32_t dwXSize;
        uint32_t dwYSize;
        uint32_t dwXCountChars;
        uint32_t dwYCountChars;
        uint32_t dwFillAttribute;
        uint32_t flags;
        uint16_t wShowWindow;
        uint16_t cbReserved2;
        unsigned char *lpReserved2;
        void *stdinput;
        void *stdoutput;
        void *stderror;
    } WinStartupInfo;

    typedef struct {
        void *process;
        void *thread;
        uint32_t process_id;
        uint32_t thread_id;
    } WinProcessInfo;

    int CreateProcessA(
        const char *application_name,
        char *command_line,
        WinSecurityAttr *process_attr,
        WinSecurityAttr *thread_attr,
        int inherit_handles,
        uint32_t creation_flags,
        void *environment,
        const char *current_directory,
        WinStartupInfo *startup_info,
        WinProcessInfo *process_info
    );

    #ifndef WIN_INFINITE
        #define WIN_INFINITE 0xffffffff
    #endif

    uint32_t WaitForSingleObject(void *handle, uint32_t timeout_ms);

    #define WIN_STD_INPUT_HANLDE ((uint32_t)-10)
    #define WIN_STD_OUTPUT_HANLDE ((uint32_t)-11)
    #define WIN_STD_ERROR_HANLDE ((uint32_t)-12)

    void *GetStdHandle(uint32_t std_handle);
    int CloseHandle(void *handle);
    int GetExitCodeProcess(void *handle, uint32_t *exit_code);
#else
    #include <sys/wait.h>
#endif

AVEN_FN AvenBuildPIDResult aven_build_cmd_run(
    AvenStrSlice cmd,
    AvenArena arena
) {
    AvenStr cmd_str = aven_str_join(
        cmd,
        ' ',
        &arena
    );

    printf("%s\n", cmd_str.ptr);
#ifdef _WIN32
    WinStartupInfo startup_info = {
        .cb = sizeof(WinStartupInfo),
        .stdinput = GetStdHandle(WIN_STD_INPUT_HANLDE),
        .stdoutput = GetStdHandle(WIN_STD_OUTPUT_HANLDE),
        .stderror = GetStdHandle(WIN_STD_ERROR_HANLDE),
        .flags = WIN_STARTF_USESTDHANDLES,
    };
    WinProcessInfo process_info = { 0 };

    int success = CreateProcessA(
        NULL,
        cmd_str.ptr,
        NULL,
        NULL,
        true,
        0,
        NULL,
        NULL,
        &startup_info,
        &process_info
    );
    if (success == 0) {
        return (AvenBuildPIDResult){ .error = AVEN_BUILD_CMD_ERROR_FORK };
    }

    CloseHandle(process_info.thread);
    
    return (AvenBuildPIDResult){ .payload = process_info.process };
#else
    AvenBuildPID cmd_pid = fork();
    if (cmd_pid < 0) {
        return (AvenBuildPIDResult){ .error = AVEN_BUILD_CMD_ERROR_FORK };
    }

    if (cmd_pid == 0) {
        char **args = aven_arena_alloc(
            &arena,
            (cmd.len + 1) * sizeof(*args),
            16
        );

        for (size_t i = 0; i < cmd.len; i += 1) {
            args[i] = slice_get(cmd, i).ptr;
        }
        args[cmd.len] = NULL;

        int error = execvp(slice_get(cmd, 0).ptr, args);
        if (error != 0) {
            fprintf(stderr, "exec failed: %s\n", cmd_str.ptr);
            exit(1);
        }
    }

    return (AvenBuildPIDResult){ .payload = cmd_pid };
#endif
}

AVEN_FN int aven_build_cmd_wait(AvenBuildPID pid) {
#ifdef _WIN32
    uint32_t result = WaitForSingleObject(pid, WIN_INFINITE);
    if (result != 0) {
        return AVEN_BUILD_WAIT_ERROR_WAIT;
    }

    uint32_t exit_code;
    int success = GetExitCodeProcess(pid, &exit_code);
    if (success == 0) {
        return AVEN_BUILD_WAIT_ERROR_GETCODE;
    }

    if (exit_code != 0) {
        return AVEN_BUILD_WAIT_ERROR_PROCESS;
    }
#else
    for (;;) {
        int wstatus = 0;
        int res_pid = waitpid(pid, &wstatus, 0);
        if (res_pid < 0) {
            return AVEN_BUILD_WAIT_ERROR_WAIT;
        }

        if (WIFEXITED(wstatus)) {
            int exit_status = WEXITSTATUS(wstatus);
            if (exit_status != 0) {
                return AVEN_BUILD_WAIT_ERROR_PROCESS;
            }

            break;
        }

        if (WIFSIGNALED(wstatus)) {
            return AVEN_BUILD_WAIT_ERROR_SIGNAL;
        }
    }
#endif

    return 0;
}

static int aven_build_step_wait(AvenBuildStep *step) {
    if (step->state != AVEN_BUILD_STEP_STATE_RUNNING) {
        return 0;
    }

    int error = aven_build_cmd_wait(step->pid);
    step->state = AVEN_BUILD_STEP_STATE_DONE;
    return error;
}

AVEN_FN int aven_build_step_run(AvenBuildStep *step, AvenArena arena) {
    if (step->state != AVEN_BUILD_STEP_STATE_NONE) {
        return 0;
    }

    for (AvenBuildStepNode *dep = step->dep; dep != NULL; dep = dep->next) {
        int error = aven_build_step_run(dep->step, arena);
        if (error != 0) {
            return AVEN_BUILD_STEP_RUN_ERROR_DEPRUN;
        }
    }

    for (AvenBuildStepNode *dep = step->dep; dep != NULL; dep = dep->next) {
        int error = aven_build_step_wait(dep->step);
        if (error != 0) {
            return AVEN_BUILD_STEP_RUN_ERROR_DEPWAIT;
        }
    }

    step->state = AVEN_BUILD_STEP_STATE_RUNNING;

    int error = 0;;
    AvenBuildPIDResult result;
    switch (step->type) {
        case AVEN_BUILD_STEP_TYPE_ROOT:
            step->state = AVEN_BUILD_STEP_STATE_DONE;
            break;
        case AVEN_BUILD_STEP_TYPE_CMD:
            result = aven_build_cmd_run(
                step->data.cmd,
                arena
            );
            if (result.error != 0) {
                return AVEN_BUILD_STEP_RUN_ERROR_CMD;
            }

            step->pid = result.payload;
            break;
        case AVEN_BUILD_STEP_TYPE_RM:
            printf("rm %s\n", step->data.rm.ptr);
            error = aven_fs_rm(step->data.rm);
            if (error != 0) {
                return AVEN_BUILD_STEP_RUN_ERROR_RM;
            }
            step->state = AVEN_BUILD_STEP_STATE_DONE;
            break;
        case AVEN_BUILD_STEP_TYPE_RMDIR:
            printf("rmdir %s\n", step->data.rmdir.ptr);
            error = aven_fs_rmdir(step->data.rmdir);
            if (error != 0) {
                return AVEN_BUILD_STEP_RUN_ERROR_RMDIR;
            }
            step->state = AVEN_BUILD_STEP_STATE_DONE;
            break;
        case AVEN_BUILD_STEP_TYPE_TRUNC:
            if (!step->out_path.valid) {
                return AVEN_BUILD_STEP_RUN_ERROR_OUTPATH;
            }
            printf("truncate -s 0 %s\n", step->out_path.value.ptr);
            error = aven_fs_trunc(step->out_path.value);
            if (error != 0) {
                return AVEN_BUILD_STEP_RUN_ERROR_TRUNC;
            }
            step->state = AVEN_BUILD_STEP_STATE_DONE;
            break;
        case AVEN_BUILD_STEP_TYPE_MKDIR:
            if (!step->out_path.valid) {
                return AVEN_BUILD_STEP_RUN_ERROR_OUTPATH;
            }
            error = aven_fs_mkdir(step->out_path.value);
            if (error != 0) {
                return AVEN_BUILD_STEP_RUN_ERROR_MKDIR;
            }
            printf("mkdir %s\n", step->out_path.value.ptr);
            step->state = AVEN_BUILD_STEP_STATE_DONE;
            break;
        case AVEN_BUILD_STEP_TYPE_COPY:
            if (!step->out_path.valid) {
                return AVEN_BUILD_STEP_RUN_ERROR_OUTPATH;
            }
            error = aven_fs_copy(
                step->data.copy,
                step->out_path.value
            );
            if (error != 0) {
                return AVEN_BUILD_STEP_RUN_ERROR_COPY;
            }
            printf("cp %s %s\n", step->data.copy.ptr, step->out_path.value.ptr);
            step->state = AVEN_BUILD_STEP_STATE_DONE;
            break;
        default:
            return AVEN_BUILD_STEP_RUN_ERROR_BADTYPE;
    }

    return 0;
}

AVEN_FN void aven_build_step_clean(AvenBuildStep *step) {
    if (step->out_path.valid) {
        aven_fs_rm(step->out_path.value);
        aven_fs_rmdir(step->out_path.value);
    }
    step->state = AVEN_BUILD_STEP_STATE_NONE;

    for (AvenBuildStepNode *dep = step->dep; dep != NULL; dep = dep->next) {
        aven_build_step_clean(dep->step);
    }
}

AVEN_FN void aven_build_step_reset(AvenBuildStep *step) {
    step->state = AVEN_BUILD_STEP_STATE_NONE;

    for (AvenBuildStepNode *dep = step->dep; dep != NULL; dep = dep->next) {
        aven_build_step_reset(dep->step);
    }
}

#endif // AVEN_IMPLEMENTATOIN

#endif // AVEN_BUILD_H
