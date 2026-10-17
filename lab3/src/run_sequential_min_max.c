#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <seed> <array_size>\n", argv[0]);
        return 1;
    }

    char **child_argv = malloc((argc + 1) * sizeof(char *));
    if (!child_argv) {
        perror("malloc");
        return 1;
    }

    child_argv[0] = "./sequential_min_max";
    for (int i = 1; i < argc; i++) {
        child_argv[i] = argv[i];
    }
    child_argv[argc] = NULL;

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        free(child_argv);
        return 1;
    }

    if (pid == 0) {
        execvp(child_argv[0], child_argv);
        perror("execvp");
        _exit(1);
    }

    // === Родительский процесс ===
    free(child_argv);  

    int status;
    if (waitpid(pid, &status, 0) == -1) {
        perror("waitpid");
        return 1;
    }

    if (WIFEXITED(status)) {
        printf("Child exited with status %d\n", WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        printf("Child killed by signal %d\n", WTERMSIG(status));
    }
    return 0;
}