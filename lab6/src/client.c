#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>
#include "mult_modulo.h"

struct Server {
  char ip[255];
  int port;
};

struct ThreadArgs {
  struct Server server;
  uint64_t begin;
  uint64_t end;
  uint64_t mod;
  uint64_t result;
};


bool ConvertStringToUI64(const char *str, uint64_t *val) {
  char *end = NULL;
  unsigned long long i = strtoull(str, &end, 10);
  if (errno == ERANGE) {
    fprintf(stderr, "Out of uint64_t range: %s\n", str);
    return false;
  }

  if (errno != 0)
    return false;

  *val = i;
  return true;
}

// ВСТАВКА КОДА - Функция для работы с сервером в отдельном потоке
void* ProcessServer(void* args) {
  struct ThreadArgs* thread_args = (struct ThreadArgs*)args;
  
  struct hostent *hostname = gethostbyname(thread_args->server.ip);
  if (hostname == NULL) {
    fprintf(stderr, "gethostbyname failed with %s\n", thread_args->server.ip);
    thread_args->result = 0;
    return NULL;
  }

  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(thread_args->server.port);
  server_addr.sin_addr.s_addr = *((unsigned long *)hostname->h_addr);

  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    fprintf(stderr, "Socket creation failed!\n");
    thread_args->result = 0;
    return NULL;
  }

  if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    fprintf(stderr, "Connection failed to %s:%d\n", 
            thread_args->server.ip, thread_args->server.port);
    close(sock);
    thread_args->result = 0;
    return NULL;
  }

  char task[sizeof(uint64_t) * 3];
  memcpy(task, &thread_args->begin, sizeof(uint64_t));
  memcpy(task + sizeof(uint64_t), &thread_args->end, sizeof(uint64_t));
  memcpy(task + 2 * sizeof(uint64_t), &thread_args->mod, sizeof(uint64_t));

  if (send(sock, task, sizeof(task), 0) < 0) {
    fprintf(stderr, "Send failed to %s:%d\n", 
            thread_args->server.ip, thread_args->server.port);
    close(sock);
    thread_args->result = 0;
    return NULL;
  }

  char response[sizeof(uint64_t)];
  ssize_t bytes_received = recv(sock, response, sizeof(response), 0);
  if (bytes_received != sizeof(uint64_t)) {
    fprintf(stderr, "Receive failed from %s:%d (got %zd bytes, expected %zu)\n", 
            thread_args->server.ip, thread_args->server.port, 
            bytes_received, sizeof(uint64_t));
    close(sock);
    thread_args->result = 0;
    return NULL;
  }

  memcpy(&thread_args->result, response, sizeof(uint64_t));
  close(sock);
  
  return NULL;
}
// КОНЕЦ ВСТАВКИ


int main(int argc, char **argv) {
  uint64_t k = -1;
  uint64_t mod = -1;
  char servers_file[255] = {'\0'}; // TODO: explain why 255 - имя файла с серверами

  while (true) {
    //int current_optind = optind ? optind : 1;

    static struct option options[] = {{"k", required_argument, 0, 0},
                                      {"mod", required_argument, 0, 0},
                                      {"servers", required_argument, 0, 0},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "", options, &option_index);

    if (c == -1)
      break;

    switch (c) {
    case 0: {
      switch (option_index) {
      case 0:
        ConvertStringToUI64(optarg, &k);
        // TODO: your code here - проверка значения
        if (k == 0) {
          fprintf(stderr, "k must be greater than 0\n");
          return 1;
        }
        break;
      case 1:
        ConvertStringToUI64(optarg, &mod);
        // TODO: your code here - проверка значения
        if (mod == 0) {
          fprintf(stderr, "mod must be greater than 0\n");
          return 1;
        }
        break;
      case 2:
        // TODO: your code here - копирование имени файла с серверами
        strncpy(servers_file, optarg, sizeof(servers_file) - 1);
        servers_file[sizeof(servers_file) - 1] = '\0';
        break;
      default:
        printf("Index %d is out of options\n", option_index);
      }
    } break;

    case '?':
      printf("Arguments error\n");
      break;
    default:
      fprintf(stderr, "getopt returned character code 0%o?\n", c);
    }
  }

  if (k == -1 || mod == -1 || !strlen(servers_file)) {
    fprintf(stderr, "Using: %s --k 1000 --mod 5 --servers /path/to/file\n",
            argv[0]);
    return 1;
  }

  // TODO: for one server here, rewrite with servers from file
  // ВСТАВКА КОДА - Чтение серверов из файла
  FILE *file = fopen(servers_file, "r");
  if (file == NULL) {
    fprintf(stderr, "Cannot open servers file: %s\n", servers_file);
    return 1;
  }

  struct Server servers[100];
  unsigned int servers_num = 0;
  char line[255];
  
  while (fgets(line, sizeof(line), file) && servers_num < 100) {
    line[strcspn(line, "\n")] = '\0';
    
    char *colon = strchr(line, ':');
    if (colon == NULL) {
      fprintf(stderr, "Invalid server format: %s (expected ip:port)\n", line);
      continue;
    }
    
    *colon = '\0';
    strncpy(servers[servers_num].ip, line, sizeof(servers[servers_num].ip) - 1);
    servers[servers_num].port = atoi(colon + 1);
    
    if (servers[servers_num].port <= 0) {
      fprintf(stderr, "Invalid port in: %s\n", line);
      continue;
    }
    
    servers_num++;
  }
  
  fclose(file);
  
  if (servers_num == 0) {
    fprintf(stderr, "No valid servers found in file: %s\n", servers_file);
    return 1;
  }
  
  printf("Found %u servers\n", servers_num);
  // КОНЕЦ ВСТАВКИ

  // TODO: work continiously, rewrite to make parallel
  // ВСТАВКА КОДА - Параллельное распределение работы между серверами
  pthread_t threads[servers_num];
  struct ThreadArgs thread_args[servers_num];
  
  uint64_t step = k / servers_num;
  uint64_t remainder = k % servers_num;
  
  for (int i = 0; i < servers_num; i++) {
    thread_args[i].server = servers[i];
    thread_args[i].mod = mod;
    
    thread_args[i].begin = i * step + 1;
    if (i < remainder) {
      thread_args[i].begin += i;
    } else {
      thread_args[i].begin += remainder;
    }
    
    thread_args[i].end = (i == servers_num - 1) ? k : thread_args[i].begin + step - 1;
    if (i < remainder) {
      thread_args[i].end += 1;
    }
    
    printf("Server %d (%s:%d) will compute [%" PRIu64 ", %" PRIu64 "]\n", 
           i, servers[i].ip, servers[i].port, 
           thread_args[i].begin, thread_args[i].end);
    
    if (pthread_create(&threads[i], NULL, ProcessServer, &thread_args[i]) != 0) {
      fprintf(stderr, "Failed to create thread for server %d\n", i);
    }
  }
  
  for (int i = 0; i < servers_num; i++) {
    pthread_join(threads[i], NULL);
  }
  
  uint64_t total = 1;
  for (int i = 0; i < servers_num; i++) {
    if (thread_args[i].result == 0) {
      fprintf(stderr, "Warning: Server %d returned 0 or failed\n", i);
    }
    total = MultModulo(total, thread_args[i].result, mod);
    printf("Partial result from server %d: %" PRIu64 " (total: %" PRIu64 ")\n",  
           i, thread_args[i].result, total);
  }
  
  printf("Final answer: %" PRIu64 "! mod %" PRIu64 " = %" PRIu64 "\n", k, mod, total);
  // КОНЕЦ ВСТАВКИ

  return 0;
}