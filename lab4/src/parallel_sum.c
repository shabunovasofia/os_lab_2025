#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <pthread.h>

#include <getopt.h>
#include "/workspaces/os_lab_2025/lab3/src/utils.h"
#include "sum_lib.h" 
// КОНЕЦ ВСТАВКИ


void *ThreadSum(void *args) {
  struct SumArgs *sum_args = (struct SumArgs *)args;
  return (void *)(size_t)Sum(sum_args);
}

// НАЧАЛО ВСТАВКИ - функция для парсинга аргументов командной строки
int parse_arguments(int argc, char **argv, uint32_t *threads_num, uint32_t *array_size, uint32_t *seed) {
    static struct option options[] = {
        {"threads_num", required_argument, 0, 0},
        {"array_size", required_argument, 0, 0},
        {"seed", required_argument, 0, 0},
        {0, 0, 0, 0}
    };

    int option_index = 0;
    while (true) {
        int c = getopt_long(argc, argv, "", options, &option_index);
        
        if (c == -1) break;

        switch (c) {
            case 0:
                switch (option_index) {
                    case 0:
                        *threads_num = atoi(optarg);
                        if (*threads_num <= 0) {
                            printf("threads_num must be a positive number\n");
                            return -1;
                        }
                        break;
                    case 1:
                        *array_size = atoi(optarg);
                        if (*array_size <= 0) {
                            printf("array_size must be a positive number\n");
                            return -1;
                        }
                        break;
                    case 2:
                        *seed = atoi(optarg);
                        break;
                    default:
                        printf("Index %d is out of options\n", option_index);
                        return -1;
                }
                break;
            case '?':
                break;
            default:
                printf("getopt returned character code 0%o?\n", c);
                return -1;
        }
    }

    if (optind < argc) {
        printf("Has at least one no option argument\n");
        return -1;
    }

    if (*threads_num == 0 || *array_size == 0) {
        printf("Usage: %s --threads_num \"num\" --seed \"num\" --array_size \"num\"\n", argv[0]);
        return -1;
    }
    
    return 0;
}
// КОНЕЦ ВСТАВКИ

int main(int argc, char **argv) {
  uint32_t threads_num = 0;
  uint32_t array_size = 0;
  uint32_t seed = 0;

  // НАЧАЛО ВСТАВКИ - парсинг аргументов
  if (parse_arguments(argc, argv, &threads_num, &array_size, &seed) != 0) {
    return 1;
  }
  // КОНЕЦ ВСТАВКИ

  printf("Threads: %u, Array size: %u, Seed: %u\n", threads_num, array_size, seed);

  // НАЧАЛО ВСТАВКИ - генерация массива 
  int *array = malloc(sizeof(int) * array_size);
  if (array == NULL) {
    printf("Memory allocation failed for array\n");
    return 1;
  }
  GenerateArray(array, array_size, seed);
  // КОНЕЦ ВСТАВКИ

  // НАЧАЛО ВСТАВКИ - создание потоков с правильным распределением работы
  pthread_t threads[threads_num];
  struct SumArgs args[threads_num];
  
  int chunk_size = array_size / threads_num;
  int remainder = array_size % threads_num;
  int current_start = 0;
  
  for (uint32_t i = 0; i < threads_num; i++) {
    int chunk = chunk_size + (i < remainder ? 1 : 0);
    args[i].array = array;
    args[i].begin = current_start;
    args[i].end = current_start + chunk;
    current_start += chunk;
    
    if (pthread_create(&threads[i], NULL, ThreadSum, (void *)&args[i])) {
      printf("Error: pthread_create failed!\n");
      free(array);
      return 1;
    }
  }
  // КОНЕЦ ВСТАВКИ

  // НАЧАЛО ВСТАВКИ - замер времени выполнения (только суммирование)
  struct timespec start, end;
  clock_gettime(CLOCK_MONOTONIC, &start);
  // КОНЕЦ ВСТАВКИ

  int total_sum = 0;
  for (uint32_t i = 0; i < threads_num; i++) {
    int sum = 0;
    pthread_join(threads[i], (void **)&sum);
    total_sum += sum;
  }

  // НАЧАЛО ВСТАВКИ - завершение замера времени и вывод результатов
  clock_gettime(CLOCK_MONOTONIC, &end);
  
  double time_taken = (end.tv_sec - start.tv_sec) * 1e9;
  time_taken = (time_taken + (end.tv_nsec - start.tv_nsec)) * 1e-9;
  
  free(array);
  printf("Total sum: %d\n", total_sum);
  printf("Time taken for parallel sum: %.6f seconds\n", time_taken);
  // КОНЕЦ ВСТАВКИ
  
  return 0;
}