#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

#include "Worker.h"

int worker_execute_command(
    int64_t job_id,
    const char *command,
    int *exit_code,
    int (*heartbeat)(void *context),
    void *context
)
{
    if (command == NULL || exit_code == NULL) {
        return 0;
    }

    printf("Worker processing job %lld: %s\n",
           (long long)job_id, command);

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        *exit_code = -1;
        return 0;
    }

    if (pid == 0) {
        if (setpgid(0, 0) < 0) {
            perror("setpgid");
            _exit(127);
        }
        execl("/bin/sh", "sh", "-c", command, (char *)NULL);
        perror("execl");
        _exit(127);
    }

    unsigned int seconds_since_heartbeat = 0;
    int status = 0;
    for (;;) {
        pid_t result = waitpid(pid, &status, WNOHANG);
        if (result == pid) {
            if (WIFEXITED(status)) {
                *exit_code = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                *exit_code = 128 + WTERMSIG(status);
            } else {
                *exit_code = -1;
            }
            return 1;
        }
        if (result < 0 && errno != EINTR) {
            perror("waitpid");
            *exit_code = -1;
            return 0;
        }

        sleep(1);
        if (heartbeat != NULL && ++seconds_since_heartbeat >= 20) {
            seconds_since_heartbeat = 0;
            if (!heartbeat(context)) {
                if (kill(-pid, SIGKILL) < 0) {
                    kill(pid, SIGKILL);
                }
                while (waitpid(pid, &status, 0) < 0 && errno == EINTR) {
                }
                *exit_code = -1;
                return 0;
            }
        }
    }
}

int worker_execute(Job *job)
{
    if (job == NULL) {
        return 0;
    }

    int exit_code = -1;
    return worker_execute_command(
        job->id,
        job->command,
        &exit_code,
        NULL,
        NULL
    ) && exit_code == 0;
}
