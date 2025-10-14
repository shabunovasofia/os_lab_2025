#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {

        execl("./sequential_min_max",
              "sequential_min_max",
              "123",
              "1000",
              (char *)NULL);


        perror("execl");
        _exit(127);
    } else {
        int status;
        if (waitpid(pid, &status, 0) == -1) {
            perror("waitpid");
            return 1;
        }
        if (WIFEXITED(status)) {
            printf("sequential_min_max exited with status %d\n", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("sequential_min_max killed by signal %d\n", WTERMSIG(status));
        } else {
            printf("sequential_min_max ended abnormally\n");
        }
    }

    return 0;
}
