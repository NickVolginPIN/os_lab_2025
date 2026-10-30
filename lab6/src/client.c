#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "libnetmath.h"

struct Server {
    char ip[255];
    int port;
};

// +++ Аргументы для одного потока-клиента
struct ClientTask {
    struct Server server;
    uint64_t begin;
    uint64_t end;
    uint64_t mod;
    uint64_t result;   // выход
    int status;        // 0 = успех, -1 = ошибка
};

// +++ Один поток = одно TCP-соединение с одним сервером
static void *ServerWorker(void *arg) {
    struct ClientTask *t = (struct ClientTask *)arg;
    t->status = -1;

    struct hostent *hostname = gethostbyname(t->server.ip);
    if (!hostname) {
        fprintf(stderr, "gethostbyname failed with %s\n", t->server.ip);
        return NULL;
    }

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons((uint16_t)t->server.port);
    server.sin_addr.s_addr = *((unsigned long *)hostname->h_addr);

    int sck = socket(AF_INET, SOCK_STREAM, 0);
    if (sck < 0) { perror("socket"); return NULL; }

    if (connect(sck, (struct sockaddr *)&server, sizeof(server)) < 0) {
        fprintf(stderr, "connect to %s:%d failed\n", t->server.ip, t->server.port);
        close(sck);
        return NULL;
    }

    char task[sizeof(uint64_t) * 3];
    memcpy(task, &t->begin, sizeof(uint64_t));
    memcpy(task + sizeof(uint64_t), &t->end, sizeof(uint64_t));
    memcpy(task + 2 * sizeof(uint64_t), &t->mod, sizeof(uint64_t));

    if (send(sck, task, sizeof(task), 0) < (ssize_t)sizeof(task)) {
        perror("send");
        close(sck);
        return NULL;
    }

    char response[sizeof(uint64_t)];
    ssize_t got = recv(sck, response, sizeof(response), 0);
    if (got != (ssize_t)sizeof(uint64_t)) {
        fprintf(stderr, "recv from %s:%d got %zd bytes\n",
                t->server.ip, t->server.port, got);
        close(sck);
        return NULL;
    }

    memcpy(&t->result, response, sizeof(uint64_t));
    t->status = 0;
    close(sck);
    return NULL;
}

// +++ Парсер строки "ip:port" — отдельная функция для читаемости
static bool ParseServerLine(const char *line, struct Server *srv) {
    const char *colon = strrchr(line, ':');
    if (!colon) return false;

    size_t ip_len = (size_t)(colon - line);
    if (ip_len == 0 || ip_len >= sizeof(srv->ip)) return false;

    memcpy(srv->ip, line, ip_len);
    srv->ip[ip_len] = '\0';

    char *end = NULL;
    errno = 0;
    long p = strtol(colon + 1, &end, 10);
    if (errno != 0 || *end != '\0' || p <= 0 || p > 65535) return false;

    srv->port = (int)p;
    return true;
}

int main(int argc, char **argv) {
    uint64_t k = 0;
    uint64_t mod = 0;
    // 255 — де-факто стандартный лимит на путь к файлу (унаследован от старых ФС,
    // напр. MAX_PATH в DOS). PATH_MAX на Linux = 4096, но 255 хватает с запасом.
    char servers_path[255] = {0};

    while (true) {
        static struct option options[] = {{"k",       required_argument, 0, 0},
                                          {"mod",     required_argument, 0, 0},
                                          {"servers", required_argument, 0, 0},
                                          {0, 0, 0, 0}};
        int option_index = 0;
        int c = getopt_long(argc, argv, "", options, &option_index);
        if (c == -1) break;

        switch (c) {
        case 0:
            switch (option_index) {
            case 0:
                if (!ConvertStringToUI64(optarg, &k)) return 1;
                break;
            case 1:
                if (!ConvertStringToUI64(optarg, &mod)) return 1;
                break;
            case 2:
                if (strlen(optarg) >= sizeof(servers_path)) {
                    fprintf(stderr, "servers path too long\n");
                    return 1;
                }
                strcpy(servers_path, optarg);
                break;
            default:
                printf("Index %d is out of options\n", option_index);
            }
            break;
        case '?':
            printf("Arguments error\n");
            break;
        default:
            fprintf(stderr, "getopt returned character code 0%o?\n", c);
        }
    }

    if (k == 0 || mod == 0 || servers_path[0] == '\0') {
        fprintf(stderr,
                "Using: %s --k 1000 --mod 5 --servers /path/to/file\n", argv[0]);
        return 1;
    }

    // +++ Читаем серверы из файла (ip:port построчно)
    FILE *f = fopen(servers_path, "r");
    if (!f) { perror("fopen servers"); return 1; }

    struct Server *to = NULL;
    unsigned int servers_num = 0;
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\r\n")] = '\0';
        if (line[0] == '\0' || line[0] == '#') continue;

        to = realloc(to, sizeof(struct Server) * (servers_num + 1));
        if (!to) { perror("realloc"); fclose(f); return 1; }

        if (!ParseServerLine(line, &to[servers_num])) {
            fprintf(stderr, "Bad server line: '%s'\n", line);
            servers_num--;   // rollback realloc
            to = realloc(to, sizeof(struct Server) * servers_num);
            continue;
        }
        servers_num++;
    }
    fclose(f);

    if (servers_num == 0) {
        fprintf(stderr, "No servers in %s\n", servers_path);
        free(to);
        return 1;
    }

    // +++ Если серверов больше, чем k — обрезаем (иначе пустые диапазоны)
    unsigned int active = servers_num;
    if ((uint64_t)active > k) active = (uint64_t)k;

    // +++ Готовим задания: k! разбит на active частей
    struct ClientTask *tasks = calloc(active, sizeof(struct ClientTask));
    pthread_t *threads = calloc(active, sizeof(pthread_t));
    if (!tasks || !threads) { perror("calloc"); return 1; }

    uint64_t chunk = k / active;
    for (unsigned int i = 0; i < active; i++) {
        tasks[i].server = to[i];
        tasks[i].begin  = (uint64_t)i * chunk + 1;
        tasks[i].end    = (i == active - 1) ? k : (uint64_t)(i + 1) * chunk;
        tasks[i].mod    = mod;
        tasks[i].result = 1;
        tasks[i].status = -1;
        if (pthread_create(&threads[i], NULL, ServerWorker, &tasks[i])) {
            fprintf(stderr, "pthread_create failed\n");
            return 1;
        }
    }

    // +++ Ждём все потоки — сервера работают параллельно
    for (unsigned int i = 0; i < active; i++) {
        pthread_join(threads[i], NULL);
    }

    // +++ Свёртка ответов: answer = Π result_i (mod mod)
    uint64_t answer = 1 % mod;
    for (unsigned int i = 0; i < active; i++) {
        if (tasks[i].status == 0) {
            answer = MultModulo(answer, tasks[i].result, mod);
        } else {
            fprintf(stderr, "Server %s:%d failed, skipping\n",
                    tasks[i].server.ip, tasks[i].server.port);
        }
    }

    printf("answer: %llu\n", (unsigned long long)answer);

    free(tasks);
    free(threads);
    free(to);
    return 0;
}