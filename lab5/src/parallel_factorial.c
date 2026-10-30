#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <getopt.h>

static long long g_k        = 0;
static long long g_mod      = 0;
static long long g_pnum     = 0;
static long long g_result   = 1;

static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

struct ThreadArgs {
    long long begin;   /* включительно, от 1 */
    long long end;     /* включительно, до k  */
};

static void *ThreadFunc(void *arg) {
    struct ThreadArgs *a = (struct ThreadArgs *)arg;
    long long local = 1;
    for (long long v = a->begin; v <= a->end; v++) {
        local = (local * (v % g_mod)) % g_mod;
    }
    /* Критическая секция — одна операция на поток */
    pthread_mutex_lock(&g_mutex);
    g_result = (g_result * local) % g_mod;
    pthread_mutex_unlock(&g_mutex);
    return NULL;
}

int main(int argc, char **argv) {
    while (1) {
        static struct option opts[] = {
            {"k",    required_argument, 0, 'k'},
            {"pnum", required_argument, 0, 'p'},
            {"mod",  required_argument, 0, 'm'},
            {0, 0, 0, 0}};
        int opt = getopt_long(argc, argv, "k:p:m:", opts, NULL);
        if (opt == -1) break;
        switch (opt) {
            case 'k': g_k    = atoll(optarg); break;
            case 'p': g_pnum = atoll(optarg); break;
            case 'm': g_mod  = atoll(optarg); break;
            default:
                fprintf(stderr, "Usage: %s -k N --pnum=N --mod=N\n", argv[0]);
                return 1;
        }
    }

    if (g_k <= 0 || g_pnum <= 0 || g_mod <= 0) {
        fprintf(stderr, "All of -k, --pnum, --mod must be positive\n");
        return 1;
    }
    if (g_pnum > g_k) g_pnum = g_k;   /* не больше диапазонов, чем чисел */

    pthread_t threads[g_pnum];
    struct ThreadArgs args[g_pnum];

    for (long long i = 0; i < g_pnum; i++) {
        args[i].begin = i * g_k / g_pnum + 1;
        args[i].end   = (i + 1) * g_k / g_pnum;
        if (pthread_create(&threads[i], NULL, ThreadFunc, &args[i]) != 0) {
            perror("pthread_create");
            return 1;
        }
    }

    for (long long i = 0; i < g_pnum; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("%lld! mod %lld = %lld\n", g_k, g_mod, g_result);
    pthread_mutex_destroy(&g_mutex);
    return 0;
}