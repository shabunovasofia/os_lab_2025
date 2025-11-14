#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#include <getopt.h>

#include "find_min_max.h"
#include "utils.h"

// Глобальные переменные для хранения PID дочерних процессов
pid_t *child_pids = NULL;
int timeout = 0; 

// Функция-обработчик сигнала таймаута
void timeout_handler(int sig) {
    if (sig == SIGALRM) {
        printf("Timeout reached! Killing child processes...\n");
        for (int i = 0; child_pids[i] != 0; i++) {
            kill(child_pids[i], SIGKILL);
        }
    }
}

int main(int argc, char **argv) {
  int seed = -1;
  int array_size = -1;
  int pnum = -1;
  bool with_files = false;

  // Добавляем обработку параметра --timeout
  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {{"seed", required_argument, 0, 0},
                                      {"array_size", required_argument, 0, 0},
                                      {"pnum", required_argument, 0, 0},
                                      {"by_files", no_argument, 0, 'f'},
                                      {"timeout", required_argument, 0, 0},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "f", options, &option_index);

    if (c == -1) break;

    switch (c) {
      case 0:
        switch (option_index) {
          case 0:
            seed = atoi(optarg);
            if(seed <= 0){
                printf("seed must be a positive number\n");
                return 1;
            }
            break;
          case 1:
            array_size = atoi(optarg);
            if(array_size <= 0){
                printf("array_size must be a positive number\n");
                return 1;
            }
            break;
          case 2:
            pnum = atoi(optarg);
            if(pnum <= 0){
                printf("pnum must be a positive number\n");
                return 1;
            }
            break;
          case 3:
            with_files = true;
            break;
          case 4: // Обработка --timeout
            timeout = atoi(optarg);
            if (timeout <= 0) {
                printf("timeout must be a positive number\n");
                return 1;
            }
            break;
          default:
            printf("Index %d is out of options\n", option_index);
        }
        break;
      case 'f':
        with_files = true;
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

  if (seed == -1 || array_size == -1 || pnum == -1) {
    printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" [--timeout \"num\"]\n",
           argv[0]);
    return 1;
  }

  // Выделяем память для хранения PID дочерних процессов
  child_pids = malloc(sizeof(pid_t) * (pnum + 1));
  if (child_pids == NULL) {
    perror("malloc");
    return 1;
  }
  memset(child_pids, 0, sizeof(pid_t) * (pnum + 1));

  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);
  int active_child_processes = 0;

  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  int pipefd[2];
  if (!with_files) {
      if (pipe(pipefd) == -1) {
        perror("pipe");
        free(array);
        free(child_pids);
        return 1;
    }
  }

  // Устанавливаем обработчик сигнала таймаута
  if (timeout > 0) {
    signal(SIGALRM, timeout_handler);
    alarm(timeout); 
  }

  for (int i = 0; i < pnum; i++) {
    pid_t child_pid = fork();
    if (child_pid >= 0) {
      // successful fork
      active_child_processes += 1;
      child_pids[i] = child_pid; // Сохраняем PID дочернего процесса
      
      if (child_pid == 0) {
        // child process
        int step = array_size / pnum;
        int rem  = array_size % pnum;
        unsigned int begin = i * step + (i < rem ? i : rem);
        unsigned int end   = begin + step + (i < rem ? 1 : 0);

        struct MinMax local = GetMinMax(array, begin, end);

        sleep(2);

        if (with_files) {
          char fname[64];
          snprintf(fname, sizeof(fname), "tmp_min_max_%d.txt", i);
          FILE *f = fopen(fname, "w");
          if (!f) { perror("fopen"); _exit(1); }
          fprintf(f, "%d %d\n", local.min, local.max);
          fclose(f);
          _exit(0);
        } else {
          close(pipefd[0]);
          ssize_t w = write(pipefd[1], &local.min, sizeof(local.min));
          if (w != sizeof(local.min)) { _exit(1); }
          w = write(pipefd[1], &local.max, sizeof(local.max));
          if (w != sizeof(local.max)) { _exit(1); }

          close(pipefd[1]);
          _exit(0);
        }
        return 0;
      }

    } else {
      printf("Fork failed!\n");
      free(array);
      free(child_pids);
      return 1;
    }
  }

  if (!with_files) {
    close(pipefd[1]);
  }

  // Ожидаем завершения дочерних процессов
  while (active_child_processes > 0) {
    int status;
    pid_t finished_pid = waitpid(-1, &status, 0);
    
    if (finished_pid > 0) {
      active_child_processes -= 1;
      
      // Удаляем PID завершенного процесса из массива
      for (int i = 0; i < pnum; i++) {
        if (child_pids[i] == finished_pid) {
          child_pids[i] = 0;
          break;
        }
      }
    }
  }

  // Отменяем будильник, если все процессы завершились до таймаута
  if (timeout > 0) {
    alarm(0);
  }

  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;

  for (int i = 0; i < pnum; i++) {
    int min = INT_MAX;
    int max = INT_MIN;

    if (with_files) {
      char fname[64];
      snprintf(fname, sizeof(fname), "tmp_min_max_%d.txt", i);
      FILE *f = fopen(fname, "r");
      if (f == NULL) {
         perror("fopen");
         continue;
      }

      if (fscanf(f, "%d %d", &min, &max) != 2) {
         fprintf(stderr, "Error reading data from %s\n", fname);
        fclose(f);
        continue;
      }

       fclose(f);
       remove(fname);
    } else {
      read(pipefd[0], &min, sizeof(min));
      read(pipefd[0], &max, sizeof(max));
    }

    if (min < min_max.min) min_max.min = min;
    if (max > min_max.max) min_max.max = max;
  }

  if (!with_files) close(pipefd[0]);

  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  free(array);
  free(child_pids);

  printf("Min: %d\n", min_max.min);
  printf("Max: %d\n", min_max.max);
  printf("Elapsed time: %fms\n", elapsed_time);
  fflush(NULL);
  return 0;
}