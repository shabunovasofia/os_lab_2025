#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <getopt.h>

long long result = 1;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    int start;
    int end;
    int mod;
    int thread_id;
} ThreadArgs;

void* calculate_factorial_part(void* arg) {
    ThreadArgs* args = (ThreadArgs*)arg;
    
    printf("Поток %d обрабатывает числа от %d до %d\n", 
           args->thread_id, args->start, args->end);
    
    long long partial = 1;
    for (int i = args->start; i <= args->end; i++) {
        printf("Поток %d умножает на %d\n", args->thread_id, i);
        partial = (partial * i) % args->mod;
    }
    
    printf("Поток %d получил частичный результат: %lld\n", 
           args->thread_id, partial);
    
    pthread_mutex_lock(&mutex);
    printf("Поток %d обновляет общий результат: %lld * %lld = ", 
           args->thread_id, result, partial);
    result = (result * partial) % args->mod;
    printf("%lld\n", result);
    pthread_mutex_unlock(&mutex);
    
    free(args);
    return NULL;
}

int main(int argc, char* argv[]) {
    int k = -1;
    int pnum = -1; 
    int mod = -1;

    while (true) {
        int current_optind = optind ? optind : 1;

        static struct option options[] = {
            {"k", required_argument, 0, 0},
            {"pnum", required_argument, 0, 0},
            {"mod", required_argument, 0, 0},
            {0, 0, 0, 0}
        };

        int option_index = 0;
        int c = getopt_long(argc, argv, "", options, &option_index);

        if (c == -1) break;

        switch (c) {
            case 0:
                switch (option_index) {
                    case 0:
                        k = atoi(optarg);
                        if (k <= 0) {
                            printf("k must be a positive number\n");
                            return 1;
                        }
                        break;
                    case 1:
                        pnum = atoi(optarg);
                        if (pnum <= 0) {
                            printf("pnum must be a positive number\n");
                            return 1;
                        }
                        break;
                    case 2:
                        mod = atoi(optarg);
                        if (mod <= 0) {
                            printf("mod must be a positive number\n");
                            return 1;
                        }
                        break;
                    default:
                        printf("Index %d is out of options\n", option_index);
                }
                break;
            case '?':
                break;
            default:
                printf("getopt returned character code 0%o?\n", c);
        }
    }

    if (optind < argc) {
        printf("Has at least one no option argument\n");
        return 1;
    }

    if (k == -1 || pnum == -1 || mod == -1) {
        printf("Usage: %s --k \"num\" --pnum \"num\" --mod \"num\"\n", argv[0]);
        return 1;
    }

    if (pnum > k) {
        pnum = k;
    }

    printf("Вычисляем %d! mod %d используя %d потоков\n", k, mod, pnum);

    pthread_t threads[pnum];
    int step = k / pnum;
    int remainder = k % pnum;
    
    for (int i = 0; i < pnum; i++) {
        ThreadArgs* args = malloc(sizeof(ThreadArgs));
        args->mod = mod;
        args->thread_id = i;
        
        args->start = i * step + 1;
        if (i < remainder) {
            args->start += i;
        } else {
            args->start += remainder;
        }
        
        args->end = (i == pnum - 1) ? k : args->start + step - 1;
        if (i < remainder) {
            args->end += 1;
        }
        
        printf("Создаем поток %d для диапазона [%d, %d]\n", 
               i, args->start, args->end);
        
        if (pthread_create(&threads[i], NULL, calculate_factorial_part, args) != 0) {
            perror("pthread_create");
            return 1;
        }
    }

    for (int i = 0; i < pnum; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("pthread_join");
            return 1;
        }
    }

    printf("\nФинальный результат: %d! mod %d = %lld\n", k, mod, result);

    pthread_mutex_destroy(&mutex);
    return 0;
}