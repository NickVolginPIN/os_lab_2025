#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(void) {
    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return 1; }

    if (pid == 0) {
        /* Ребёнок завершается немедленно */
        printf("Child:  PID = %d, PPID = %d — exiting now\n",
               getpid(), getppid());
        exit(0);
    }

    /* Родитель продолжает работать и НЕ вызывает wait() сразу */
    printf("Parent: PID = %d, child PID = %d\n", getpid(), pid);
    printf("Child is now a ZOMBIE for 20 seconds.\n");
    printf("Run in another terminal:\n");
    printf("  ps -o pid,ppid,stat,cmd -p %d\n", pid);
    printf("Look for STAT = Z (zombie / defunct).\n");
    printf("Waiting 20 seconds...\n");
    sleep(20);

    /* Только теперь пожинаем ребёнка — зомби исчезает */
    int status;
    waitpid(pid, &status, 0);
    printf("Parent: child reaped, zombie gone.\n");
    return 0;
}