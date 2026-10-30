#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>          // +++
#include <errno.h>           // +++

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <getopt.h>

#include "find_min_max.h"
#include "utils.h"

/* +++ Глобалы для обработчика сигнала. sig_atomic_t — единственный тип,
 *     который безопасно читать/писать в обработчике. */
static pid_t *g_child_pids = NULL;
static volatile sig_atomic_t g_num_children = 0;
static volatile sig_atomic_t g_timed_out = 0;

/* +++ Обработчик SIGALRM: убивает всех детей SIGKILL'ом.
 *     В обработчике нельзя printf — только async-signal-safe вызовы,
 *     а kill() как раз безопасен. */
static void alarm_handler(int sig) {
    (void)sig;
    g_timed_out = 1;
    for (int i = 0; i < g_num_children; i++) {
        kill(g_child_pids[i], SIGKILL);
    }
}

int main(int argc, char **argv) {
  int seed = -1;
  int array_size = -1;
  int pnum = -1;
  int timeout = 0;            // +++ 0 = без таймаута
  bool with_files = false;

  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {
        {"seed",        required_argument, 0, 0},
        {"array_size",  required_argument, 0, 0},
        {"pnum",        required_argument, 0, 0},
        {"timeout",     required_argument, 0, 0},   // +++
        {"by_files",    no_argument,       0, 'f'},
        {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "f", options, &option_index);

    if (c == -1) break;

    switch (c) {
      case 0:
        switch (option_index) {
          case 0:
            seed = atoi(optarg);
            if (seed <= 0) { printf("seed must be positive\n"); return 1; }
            break;
          case 1:
            array_size = atoi(optarg);
            if (array_size <= 0) { printf("array_size must be positive\n"); return 1; }
            break;
          case 2:
            pnum = atoi(optarg);
            if (pnum <= 0) { printf("pnum must be positive\n"); return 1; }
            break;
          case 3:                                              // +++
            timeout = atoi(optarg);
            if (timeout <= 0) { printf("timeout must be positive\n"); return 1; }
            break;
          case 4:
            with_files = true;
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

  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);
  int active_child_processes = 0;

  int pipes[pnum][2];
  char file_names[pnum][64];

  /* +++ Ставим обработчик ДО alarm, массив PID — заранее */
  if (timeout > 0) {
    g_child_pids = malloc(sizeof(pid_t) * pnum);
    signal(SIGALRM, alarm_handler);
  }

  fflush(NULL);

  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  for (int i = 0; i < pnum; i++) {
    if (!with_files) {
      if (pipe(pipes[i]) == -1) { perror("pipe"); return 1; }
    }
    if (with_files) {
      snprintf(file_names[i], sizeof(file_names[i]), "min_max_%d.txt", i);
    }

    pid_t child_pid = fork();
    if (child_pid >= 0) {
      active_child_processes += 1;
      if (child_pid == 0) {
        int left  = i * array_size / pnum;
        int right = (i + 1) * array_size / pnum;
        struct MinMax min_max = GetMinMax(array, left, right);

        if (with_files) {
          FILE *f = fopen(file_names[i], "w");
          if (f) {
            fprintf(f, "%d %d\n", min_max.min, min_max.max);
            fclose(f);
          }
        } else {
          close(pipes[i][0]);
          write(pipes[i][1], &min_max, sizeof(min_max));
          close(pipes[i][1]);
        }
        _exit(0);
      } else {
        if (timeout > 0) g_child_pids[i] = child_pid;   // +++ сохраняем PID
        if (!with_files) close(pipes[i][1]);
      }
    } else {
      printf("Fork failed!\n");
      return 1;
    }
  }

  /* +++ Таймер стартует ПОСЛЕ fork — дети гарантированно созданы */
  if (timeout > 0) {
    g_num_children = pnum;
    alarm(timeout);
  }

  /* ~~~ Ждём с WNOHANG: SIGALRM прерывает usleep → handler убивает детей →
   *     на следующей итерации waitpid их пожинает. */
  while (active_child_processes > 0) {
    int status;
    pid_t pid = waitpid(-1, &status, WNOHANG);
    if (pid > 0) {
      active_child_processes -= 1;
    } else if (pid == -1) {
      if (errno == EINTR) continue;          // прерваны SIGALRM
      perror("waitpid");
      break;
    } else {
      usleep(1000);                          // никто пока не завершился
    }
  }

  if (timeout > 0) {
    alarm(0);                                // снимаем таймер
    free(g_child_pids);
  }

  if (g_timed_out) {
    printf("Timeout exceeded, children killed with SIGKILL\n");
  }

  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;

  for (int i = 0; i < pnum; i++) {
    int min = INT_MAX;
    int max = INT_MIN;

    if (with_files) {
      FILE *f = fopen(file_names[i], "r");
      if (f) {
        /* ~~~ Если ребёнок убит до записи — fscanf вернёт не 2 */
        if (fscanf(f, "%d %d", &min, &max) != 2) {
          min = INT_MAX;
          max = INT_MIN;
        }
        fclose(f);
        remove(file_names[i]);
      }
    } else {
      struct MinMax child_mm;
      /* ~~~ Если ребёнка убили, read вернёт 0 (EOF) — пропускаем */
      ssize_t n = read(pipes[i][0], &child_mm, sizeof(child_mm));
      close(pipes[i][0]);
      if (n == (ssize_t)sizeof(child_mm)) {
        min = child_mm.min;
        max = child_mm.max;
      }
    }

    if (min < min_max.min) min_max.min = min;
    if (max > min_max.max) min_max.max = max;
  }

  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  free(array);

  printf("Min: %d\n", min_max.min);
  printf("Max: %d\n", min_max.max);
  printf("Elapsed time: %fms\n", elapsed_time);
  fflush(NULL);
  return 0;
}