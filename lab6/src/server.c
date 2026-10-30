#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <getopt.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <pthread.h>

#include "libnetmath.h"          // ← MultModulo / ConvertStringToUI64

struct FactorialArgs {
  uint64_t begin;
  uint64_t end;
  uint64_t mod;
};

uint64_t Factorial(const struct FactorialArgs *args) {
  // +++ Если диапазон пустой (begin > end) — возвращаем мультипликативную единицу
  if (args->begin > args->end) return 1 % args->mod;
  uint64_t ans = 1;
  for (uint64_t i = args->begin; i <= args->end; i++) {
    ans = MultModulo(ans, i, args->mod);
  }
  return ans;
}

void *ThreadFactorial(void *args) {
  struct FactorialArgs *fargs = (struct FactorialArgs *)args;
  // Приводим uint64_t → uintptr_t → void*, а не к (uint64_t*): это корректный
  // способ «упаковать» число в указатель (на 64-bit оба 8 байт).
  return (void *)(uintptr_t)Factorial(fargs);
}

int main(int argc, char **argv) {
  int tnum = -1;
  int port = -1;

  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {{"port", required_argument, 0, 0},
                                      {"tnum", required_argument, 0, 0},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "", options, &option_index);
    if (c == -1) break;

    switch (c) {
    case 0:
      switch (option_index) {
      case 0:
        port = atoi(optarg);
        // +++
        if (port <= 0 || port > 65535) {
          fprintf(stderr, "port must be in [1, 65535]\n");
          return 1;
        }
        break;
      case 1:
        tnum = atoi(optarg);
        // +++
        if (tnum <= 0) {
          fprintf(stderr, "tnum must be positive\n");
          return 1;
        }
        break;
      default:
        printf("Index %d is out of options\n", option_index);
      }
      break;
    case '?':
      printf("Unknown argument\n");
      break;
    default:
      fprintf(stderr, "getopt returned character code 0%o?\n", c);
    }
  }

  if (port == -1 || tnum == -1) {
    fprintf(stderr, "Using: %s --port 20001 --tnum 4\n", argv[0]);
    return 1;
  }

  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) { fprintf(stderr, "Can not create server socket!\n"); return 1; }

  struct sockaddr_in server;
  server.sin_family = AF_INET;
  server.sin_port = htons((uint16_t)port);
  server.sin_addr.s_addr = htonl(INADDR_ANY);

  int opt_val = 1;
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(opt_val));

  if (bind(server_fd, (struct sockaddr *)&server, sizeof(server)) < 0) {
    fprintf(stderr, "Can not bind to socket!\n"); return 1;
  }
  if (listen(server_fd, 128) < 0) {
    fprintf(stderr, "Could not listen on socket\n"); return 1;
  }

  printf("Server listening at %d\n", port);
  fflush(stdout);

  while (true) {
    struct sockaddr_in client;
    socklen_t client_len = sizeof(client);
    int client_fd = accept(server_fd, (struct sockaddr *)&client, &client_len);
    if (client_fd < 0) { fprintf(stderr, "Could not establish new connection\n"); continue; }

    while (true) {
      unsigned int buffer_size = sizeof(uint64_t) * 3;
      char from_client[buffer_size];
      int n = recv(client_fd, from_client, buffer_size, 0);   // ~~~ read → n
      if (n == 0) break;
      if (n < 0) { fprintf(stderr, "Client read failed\n"); break; }
      if ((unsigned int)n < buffer_size) { fprintf(stderr, "Client send wrong data format\n"); break; }

      uint64_t begin = 0, end = 0, mod = 0;
      memcpy(&begin, from_client, sizeof(uint64_t));
      memcpy(&end,   from_client + sizeof(uint64_t), sizeof(uint64_t));
      memcpy(&mod,   from_client + 2 * sizeof(uint64_t), sizeof(uint64_t));

      fprintf(stdout, "Receive: %llu %llu %llu\n",
              (unsigned long long)begin, (unsigned long long)end, (unsigned long long)mod);
      fflush(stdout);

      // +++ mod == 0 — деление на ноль в MultModulo; отвечаем нулём и идём дальше
      if (mod == 0) {
        uint64_t zero = 0;
        send(client_fd, &zero, sizeof(zero), 0);
        continue;
      }

      // +++ Пустой диапазон — сразу отвечаем 1 % mod
      if (begin > end) {
        uint64_t one = 1 % mod;
        send(client_fd, &one, sizeof(one), 0);
        continue;
      }

      // +++ Распределение диапазона между потоками
      uint64_t total_range = end - begin + 1;
      int actual_tnum = tnum;
      if ((uint64_t)actual_tnum > total_range) actual_tnum = (int)total_range;

      pthread_t threads[actual_tnum];
      struct FactorialArgs args[actual_tnum];
      uint64_t chunk = total_range / (uint64_t)actual_tnum;

      for (int i = 0; i < actual_tnum; i++) {
        args[i].begin = begin + (uint64_t)i * chunk;
        args[i].end   = (i == actual_tnum - 1)
                          ? end
                          : (begin + (uint64_t)(i + 1) * chunk - 1);
        args[i].mod   = mod;
        if (pthread_create(&threads[i], NULL, ThreadFactorial, &args[i])) {
          printf("Error: pthread_create failed!\n");
          return 1;
        }
      }

      uint64_t total = 1;
      for (int i = 0; i < actual_tnum; i++) {
        void *ret = NULL;
        pthread_join(threads[i], &ret);
        uint64_t result = (uint64_t)(uintptr_t)ret;
        total = MultModulo(total, result, mod);
      }

      printf("Total: %llu\n", (unsigned long long)total);
      fflush(stdout);

      if (send(client_fd, &total, sizeof(total), 0) < 0) {
        fprintf(stderr, "Can't send data to client\n");
        break;
      }
    }

    shutdown(client_fd, SHUT_RDWR);
    close(client_fd);
  }

  return 0;
}