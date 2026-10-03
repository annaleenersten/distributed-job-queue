#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

#include "Worker.h"

int worker_execute(Job *job)
{
    if (job == NULL) {
        return 0;
    }

    printf(
        "Worker processing job %d: %s\n",
        job->id,
        job->command
    );

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return 0;
    }

    if (pid == 0) {
        execl(
            "/bin/sh",
            "sh",
            "-c",
            job->command,
            (char *)NULL
        );

        perror("execl");
        _exit(1);
    }

    int status;

    if (waitpid(pid, &status, 0) == -1) {
        perror("waitpid");
        return 0;
    }

    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}