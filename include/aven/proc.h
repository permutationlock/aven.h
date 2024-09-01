#ifndef AVEN_PROCESS_H
#define AVEN_PROCESS_H

#include "../aven.h"
#include "arena.h"
#include "str.h"

#ifdef _WIN32
    typedef void *AvenProcId;

    __declspec(dllimport) int TerminateProcess(
        AvenProcId pid,
        unsigned int error_code
    );
#else
    typedef int AvenProcId;
#endif

typedef Result(AvenProcId) AvenProcIdResult;

typedef enum {
    AVEN_PROC_CMD_ERROR_NONE = 0,
    AVEN_PROC_CMD_ERROR_FORK,
} AvenProcCmdError;

AVEN_FN AvenProcIdResult aven_proc_cmd(AvenStrSlice cmd, AvenArena arena);

typedef enum {
    AVEN_PROC_WAIT_ERROR_NONE = 0,
    AVEN_PROC_WAIT_ERROR_WAIT,
    AVEN_PROC_WAIT_ERROR_GETCODE,
    AVEN_PROC_WAIT_ERROR_PROCESS,
    AVEN_PROC_WAIT_ERROR_SIGNAL,
} AvenProcWaitError;

AVEN_FN int aven_proc_wait(AvenProcId pid);

typedef enum {
    AVEN_PROC_KILL_ERROR_NONE = 0,
    AVEN_PROC_KILL_ERROR_KILL,
    AVEN_PROC_KILL_ERROR_OTHER,
} AvenProcKillError;

AVEN_FN int aven_proc_kill(AvenProcId pid);

#ifdef AVEN_IMPLEMENTATION

#ifdef _WIN32
    typedef struct {
        uint32_t len;
        void *security_descriptor;
        int inherit_handle;
    } AvenWinSecurityAttr;

    #define AVEN_WIN_STARTF_USESTDHANDLES 0x00000100

    typedef struct {
        uint32_t cb;
        char *reserved;
        char *desktop;
        char *title;
        uint32_t x;
        uint32_t y;
        uint32_t x_size;
        uint32_t y_size;
        uint32_t x_count_chars;
        uint32_t y_count_chars;
        uint32_t fill_attribute;
        uint32_t flags;
        uint16_t show_window;
        uint16_t breserved2;
        unsigned char *preserved2;
        void *stdinput;
        void *stdoutput;
        void *stderror;
    } AvenWinStartupInfo;

    typedef struct {
        void *process;
        void *thread;
        uint32_t process_id;
        uint32_t thread_id;
    } AvenWinProcessInfo;

    __declspec(dllimport) int CreateProcessA(
        const char *application_name,
        char *command_line,
        AvenWinSecurityAttr *process_attr,
        AvenWinSecurityAttr *thread_attr,
        int inherit_handles,
        uint32_t creation_flags,
        void *environment,
        const char *current_directory,
        AvenWinStartupInfo *startup_info,
        AvenWinProcessInfo *process_info
    );

    #ifndef AVEN_WIN_INFINITE
        #define AVEN_WIN_INFINITE 0xffffffff
    #endif

    __declspec(dllimport) uint32_t WaitForSingleObject(
        void *handle,
        uint32_t timeout_ms
    );

    #define AVEN_WIN_STD_INPUT_HANLDE ((uint32_t)-10)
    #define AVEN_WIN_STD_OUTPUT_HANLDE ((uint32_t)-11)
    #define AVEN_WIN_STD_ERROR_HANLDE ((uint32_t)-12)

    __declspec(dllimport) void *GetStdHandle(uint32_t std_handle);
    __declspec(dllimport) int CloseHandle(void *handle);
    __declspec(dllimport) int GetExitCodeProcess(
        void *handle,
        uint32_t *exit_code
    );
#else
    #ifndef _POSIX_C_SOURCE
        #error "kill requires _POSIX_C_SOURCE"
    #endif
    #include <signal.h>

    #include <sys/wait.h>
    #include <unistd.h>
#endif

AVEN_FN AvenProcIdResult aven_proc_cmd(
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
    AvenWinStartupInfo startup_info = {
        .cb = sizeof(AvenWinStartupInfo),
        .stdinput = GetStdHandle(AVEN_WIN_STD_INPUT_HANLDE),
        .stdoutput = GetStdHandle(AVEN_WIN_STD_OUTPUT_HANLDE),
        .stderror = GetStdHandle(AVEN_WIN_STD_ERROR_HANLDE),
        .flags = AVEN_WIN_STARTF_USESTDHANDLES,
    };
    AvenWinProcessInfo process_info = { 0 };

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
        return (AvenProcIdResult){ .error = AVEN_PROC_CMD_ERROR_FORK };
    }

    (void)CloseHandle(process_info.thread);
    
    return (AvenProcIdResult){ .payload = process_info.process };
#else
    AvenProcId cmd_pid = fork();
    if (cmd_pid < 0) {
        return (AvenProcIdResult){ .error = AVEN_PROC_CMD_ERROR_FORK };
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

    return (AvenProcIdResult){ .payload = cmd_pid };
#endif
}

AVEN_FN int aven_proc_wait(AvenProcId pid) {
#ifdef _WIN32
    uint32_t result = WaitForSingleObject(pid, AVEN_WIN_INFINITE);
    if (result != 0) {
        return AVEN_PROC_WAIT_ERROR_WAIT;
    }

    uint32_t exit_code;
    int success = GetExitCodeProcess(pid, &exit_code);
    if (success == 0) {
        return AVEN_PROC_WAIT_ERROR_GETCODE;
    }

    if (exit_code != 0) {
        return AVEN_PROC_WAIT_ERROR_PROCESS;
    }
#else
    for (;;) {
        int wstatus = 0;
        int res_pid = waitpid(pid, &wstatus, 0);
        if (res_pid < 0) {
            return AVEN_PROC_WAIT_ERROR_WAIT;
        }

        if (WIFEXITED(wstatus)) {
            int exit_status = WEXITSTATUS(wstatus);
            if (exit_status != 0) {
                return AVEN_PROC_WAIT_ERROR_PROCESS;
            }

            break;
        }

        if (WIFSIGNALED(wstatus)) {
            return AVEN_PROC_WAIT_ERROR_SIGNAL;
        }
    }
#endif

    return 0;
}

AVEN_FN int aven_proc_kill(AvenProcId pid) {
#ifdef _WIN32
    int success = TerminateProcess(pid, 1);
    if (success == 0) {
        (void)CloseHandle(pid);
        return AVEN_PROC_KILL_ERROR_KILL;
    }
    (void)CloseHandle(pid);
    return 0;
#else
    int error = kill(pid, SIGTERM);
    switch (error) {
        case 0:
            return 0;
        case EPERM:
        case ESRCH:
            return AVEN_PROC_KILL_ERROR_KILL;
        default:
            return AVEN_PROC_KILL_ERROR_OTHER;
    }
#endif
}

#endif // AVEN_IMPLEMENTATION

#endif // AVEN_PROCESS_H
