#ifndef AVEN_BUILD_H
#define AVEN_BUILD_H

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>

#include <aven.h>
#include "arena.h"
#include "str.h"

#define AVEN_BUILD_MAX_ARGS 32

#ifdef _WIN32
    //typedef void *AvenBuildFD;
    typedef void *AvenBuildPID;

    #define AVEN_BUILD_PATH_SEP '\\'

    typedef struct {
        uint32_t len;
        void *security_descriptor;
        int inherit_handle;
    } WinSecurityAttr;

    //int CreatePipeA(
    //    AvenBuildFD *read_fd,
    //    AvenBuildFD *write_fd,
    //    WinSecurityAttr *security_attr,
    //    uint32_t size
    //);

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

    int remove(const char *filepath);
    int _rmdir(const char *dirpath);
    int _mkdir(const char *dirpath);
#else
    #include <sys/stat.h>
    #include <sys/types.h>
    #include <sys/wait.h>
    #include <unistd.h>

    //typedef int AvenBuildFD;
    typedef pid_t AvenBuildPID;

    #define AVEN_BUILD_PATH_SEP '/'
#endif

typedef enum {
    AVEN_BUILD_STEP_STATE_NONE = 0,
    AVEN_BUILD_STEP_STATE_RUNNING,
    AVEN_BUILD_STEP_STATE_DONE,
} AvenBuildStepState;

typedef enum {
    AVEN_BUILD_STEP_TYPE_CMD,
    AVEN_BUILD_STEP_TYPE_RM,
    AVEN_BUILD_STEP_TYPE_RMDIR,
    AVEN_BUILD_STEP_TYPE_TOUCH,
    AVEN_BUILD_STEP_TYPE_MKDIR,
} AvenBuildStepType;

typedef Slice(char *) AvenBuildCmdSlice;

typedef union {
    AvenBuildCmdSlice cmd;
    char *rm;
    char *rmdir;
} AvenBuildStepData;

typedef struct AvenBuildStepNode AvenBuildStepNode;

typedef Optional(char *) AvenBuildOptionalPath;

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

typedef Result(AvenBuildPID) AvenBuildPIDResult;
typedef enum {
    AVEN_BUILD_CMD_ERROR_NONE = 0,
    AVEN_BUILD_CMD_ERROR_ALLOC,
    AVEN_BUILD_CMD_ERROR_FORK,
} AvenBuildRunCmdError;

static AvenBuildPIDResult aven_build_cmd_run(
    AvenBuildCmdSlice cmd,
    AvenArena arena
) {
    AvenStrSlice cmd_slice = { .len = cmd.len };
    cmd_slice.ptr = aven_arena_create_array(AvenStr, &arena, cmd_slice.len);
    if (cmd_slice.ptr == NULL) {
        return (AvenBuildPIDResult){ .error = AVEN_BUILD_CMD_ERROR_ALLOC };
    }

    for (size_t i = 0; i < cmd.len; i += 1) {
        slice_get(cmd_slice, i) = aven_str_from_cstr(slice_get(cmd, i));
    }

    AvenStrResult join_result = aven_str_join(
        cmd_slice,
        ' ',
        &arena
    );
    if (join_result.error != 0) {
        return (AvenBuildPIDResult){ .error = AVEN_BUILD_CMD_ERROR_ALLOC };
    }

    AvenStr cmd_str = join_result.payload;

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
    if (success != 0) {
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
        if (args == NULL) {
            fprintf(stderr, "out of memory: %s\n", cmd_str.ptr);
            exit(1);
        }

        for (size_t i = 0; i < cmd.len; i += 1) {
            args[i] = slice_get(cmd, i);
        }
        args[cmd.len] = NULL;

        int error = execvp(slice_get(cmd, 0), args);
        if (error != 0) {
            fprintf(stderr, "exec failed: %s\n", cmd_str.ptr);
            exit(1);
        }
    }

    return (AvenBuildPIDResult){ .payload = cmd_pid };
#endif
}

typedef enum {
    AVEN_BUILD_WAIT_ERROR_NONE = 0,
    AVEN_BUILD_WAIT_ERROR_WAIT,
    AVEN_BUILD_WAIT_ERROR_GETCODE,
    AVEN_BUILD_WAIT_ERROR_PROCESS,
    AVEN_BUILD_WAIT_ERROR_SIGNAL,
} AvenBuildWaitError;

int aven_build_wait(AvenBuildPID pid) {
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

static inline AvenBuildStep aven_build_step_cmd_from_slice(
    AvenBuildOptionalPath out_path,
    AvenBuildCmdSlice cmd_slice
) {
    return (AvenBuildStep){
        .type = AVEN_BUILD_STEP_TYPE_CMD,
        .data = { .cmd = cmd_slice },
        .out_path = out_path,
    };
}

static inline AvenBuildStep aven_build_step_cmd(
    AvenBuildOptionalPath out_path,
    char *cmd_str,
    ...
) {
    char *cmd_data[AVEN_BUILD_MAX_ARGS];
    AvenBuildCmdSlice cmd = { .len = 0, .ptr = cmd_data };

    cmd_data[0] = cmd_str;
    cmd.len += 1;

    va_list args;
    va_start(args, cmd_str);
    for (
        char *cstr = va_arg(args, char *);
        cstr != NULL;
        cstr = va_arg(args, char *)
    ) {
        cmd_data[cmd.len] = cstr;
        cmd.len += 1;
    }
    va_end(args);

    return aven_build_step_cmd_from_slice(out_path, cmd);
}

static inline AvenBuildStep aven_build_step_mkdir(char *dir_path) {
    return (AvenBuildStep){
        .type = AVEN_BUILD_STEP_TYPE_MKDIR,
        .out_path = { .valid = true, .value = dir_path },
    };
}

static inline AvenBuildStep aven_build_step_touch(char *file_path) {
    return (AvenBuildStep){
        .type = AVEN_BUILD_STEP_TYPE_TOUCH,
        .out_path = { .valid = true, .value = file_path },
    };
}

static inline AvenBuildStep aven_build_step_rm(char *file_path) {
    return (AvenBuildStep){
        .type = AVEN_BUILD_STEP_TYPE_RM,
        .data = { .rm = file_path },
    };
}

static inline  AvenBuildStep aven_build_step_rmdir(char *dir_path) {
    return (AvenBuildStep){
        .type = AVEN_BUILD_STEP_TYPE_RMDIR,
        .data = { .rmdir = dir_path },
    };
}

typedef enum {
    AVEN_BUILD_STEP_ADD_DEP_ERROR_NONE = 0,
    AVEN_BUILD_STEP_ADD_DEP_ERROR_ALLOC,
} AvenBuildStepAddDepError;

static void aven_build_step_add_dep(
    AvenBuildStep *step,
    AvenBuildStep *dep,
    AvenArena *arena
) {
    AvenBuildStepNode *node = aven_arena_create(
        AvenBuildStepNode,
        arena
    );
    assert(node != NULL);

    *node = (AvenBuildStepNode){
        .next = step->dep,
        .step = dep,
    };
    step->dep = node;
}

static void aven_build_rmdir(char *file) {
#ifdef _WIN32
    _rmdir(file);
#else
    rmdir(file);
#endif
}

static int aven_build_step_wait(AvenBuildStep *step) {
    if (step->state != AVEN_BUILD_STEP_STATE_RUNNING) {
        return 0;
    }

    int error = aven_build_wait(step->pid);
    step->state = AVEN_BUILD_STEP_STATE_DONE;
    return error;
}

typedef enum {
    AVEN_BUILD_STEP_RUN_ERROR_NONE = 0,
    AVEN_BUILD_STEP_RUN_ERROR_DEPRUN,
    AVEN_BUILD_STEP_RUN_ERROR_DEPWAIT,
    AVEN_BUILD_STEP_RUN_ERROR_CMD,
    AVEN_BUILD_STEP_RUN_ERROR_RM,
    AVEN_BUILD_STEP_RUN_ERROR_RMDIR,
    AVEN_BUILD_STEP_RUN_ERROR_TOUCH,
    AVEN_BUILD_STEP_RUN_ERROR_MKDIR,
    AVEN_BUILD_STEP_RUN_ERROR_OUTPATH,
    AVEN_BUILD_STEP_RUN_ERROR_BADTYPE,
} AvenBuildStepRunError;

static int aven_build_step_run(AvenBuildStep *step, AvenArena arena) {
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

    AvenBuildPIDResult result;
    switch (step->type) {
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
            printf("rm %s\n", step->data.rm);
            remove(step->data.rm);
            step->state = AVEN_BUILD_STEP_STATE_DONE;
            break;
        case AVEN_BUILD_STEP_TYPE_RMDIR:
            printf("rmdir %s\n", step->data.rmdir);
            aven_build_rmdir(step->data.rmdir);
            step->state = AVEN_BUILD_STEP_STATE_DONE;
            break;
        case AVEN_BUILD_STEP_TYPE_TOUCH:
            if (!step->out_path.valid) {
                return AVEN_BUILD_STEP_RUN_ERROR_OUTPATH;
            }
            printf("touch %s\n", step->out_path.value);
            FILE *file = fopen(step->out_path.value, "w");
            if (file == NULL) {
                return AVEN_BUILD_STEP_RUN_ERROR_TOUCH;
            }
            fclose(file);
            step->state = AVEN_BUILD_STEP_STATE_DONE;
            break;
        case AVEN_BUILD_STEP_TYPE_MKDIR:
            if (!step->out_path.valid) {
                return AVEN_BUILD_STEP_RUN_ERROR_OUTPATH;
            }
#ifdef _WIN32
            int error = _mkdir(step->out_path.value);
#else
            int error = mkdir(
                step->out_path.value,
                0755
            );
#endif
            if (error != 0 && errno != EEXIST) {
                return AVEN_BUILD_STEP_RUN_ERROR_MKDIR;
            }
            if (error == 0) {
                printf("mkdir %s\n", step->out_path.value);
            }
            step->state = AVEN_BUILD_STEP_STATE_DONE;
            break;
        default:
            return AVEN_BUILD_STEP_RUN_ERROR_BADTYPE;
    }

    return 0;
}

static void aven_build_step_clean(AvenBuildStep *step) {
    if (step->out_path.valid) {
        remove(step->out_path.value);
        aven_build_rmdir(step->out_path.value);
    }
    step->state = AVEN_BUILD_STEP_STATE_NONE;

    for (AvenBuildStepNode *dep = step->dep; dep != NULL; dep = dep->next) {
        aven_build_step_clean(dep->step);
    }
}

static void aven_build_step_reset(AvenBuildStep *step) {
    step->state = AVEN_BUILD_STEP_STATE_NONE;

    for (AvenBuildStepNode *dep = step->dep; dep != NULL; dep = dep->next) {
        aven_build_step_reset(dep->step);
    }
}

static char *aven_build_path(
    AvenArena *arena,
    char *path_str,
    ...
) {
    AvenStr path_data[AVEN_BUILD_MAX_ARGS];
    AvenStrSlice path = { .len = 0, .ptr = path_data };

    path_data[0] = aven_str_from_cstr(path_str);
    path.len += 1;

    va_list args;
    va_start(args, path_str);
    for (
        char *cstr = va_arg(args, char *);
        cstr != NULL;
        cstr = va_arg(args, char *)
    ) {
        path_data[path.len] = aven_str_from_cstr(cstr);
        path.len += 1;
    }
    va_end(args);

    AvenStrResult result = aven_str_join(
        path,
        AVEN_BUILD_PATH_SEP,
        arena
    );
    assert(result.error == 0);

    return result.payload.ptr;
}

//typedef struct {
//    AvenBuildFD read_fd;
//    AvenBuildFD write_fd;
//} AvenBuildPipe;
//
//typedef Result(AvenBuildPipe) AvenBuildPipeResult;
//
//typedef enum {
//    AVEN_BUILD_PIPE_ERROR_NONE = 0,
//    AVEN_BUILD_PIPE_ERROR_CREATE,
//} AvenBuildPipeError;
//
//static AvenBuildPipeResult aven_build_pipe(void) {
//    AvenBuildPipe p = { 0 };
//#ifdef _WIN32
//    WinSecurityAttributes sec_attr = {
//        .len = sizeof(sec_attr),
//        .inherit_handle = (int)true,
//    };
//    int success = CreatePipe(&p.read, &p.write, &sec_attr, 0);
//    if (success == 0) {
//        return (AvenBuildPipeResult){ .error = AVEN_BUILD_PIPE_ERROR_CREATE };
//    }
//#else
//    int error = pipe(&p.read_fd);
//    if (error != 0) {
//        return (AvenBuildPipeResult){ .error = AVEN_BUILD_PIPE_ERROR_CREATE };
//    }
//#endif
//    return (AvenBuildPipeResult){ .payload = p };
//}

#endif // AVEN_BUILD_H
