#ifndef AVEN_PROCESS_H
#define AVEN_PROCESS_H

#include "../aven.h"
#include "arena.h"
#include "str.h"

#ifdef _WIN32
    typedef void *AvenProcId;

    int TerminateProcess(AvenProcId pid, unsigned int error_code);
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

    #ifndef AVEN_WIN_INFINITE
        #define AVEN_WIN_INFINITE 0xffffffff
    #endif

    uint32_t WaitForSingleObject(void *handle, uint32_t timeout_ms);

    #define WIN_STD_INPUT_HANLDE ((uint32_t)-10)
    #define WIN_STD_OUTPUT_HANLDE ((uint32_t)-11)
    #define WIN_STD_ERROR_HANLDE ((uint32_t)-12)

    void *GetStdHandle(uint32_t std_handle);
    int CloseHandle(void *handle);
    int GetExitCodeProcess(void *handle, uint32_t *exit_code);
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
