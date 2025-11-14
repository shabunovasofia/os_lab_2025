#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    printf("Демонстрация зомби-процессов\n");
    
    pid_t pid = fork();
    
    if (pid == 0) {
        printf("Дочерний процесс: PID = %d, PPID = %d\n", getpid(), getppid());
        printf("Дочерний процесс завершается...\n");
        exit(0);
    } else if (pid > 0) {
        printf("Родительский процесс: PID = %d, создал дочерний с PID = %d\n", getpid(), pid);
        printf("Родительский процесс спит 30 секунд и НЕ вызывает wait()...\n");
        
        sleep(30);
        
        printf("Родительский процесс завершается...\n");
    } else {
        perror("Ошибка при создании процесса");
        return 1;
    }
    
    return 0;
}
