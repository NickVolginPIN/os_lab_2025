#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

static pthread_mutex_t g_mutex_a = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_mutex_b = PTHREAD_MUTEX_INITIALIZER;

static void *ThreadAB(void *arg) {
    (void)arg;
    printf("Thread AB: locking A...\n");
    pthread_mutex_lock(&g_mutex_a);
    printf("Thread AB: A locked, sleeping...\n");
    usleep(100000);   /* окно для потока BA */

    printf("Thread AB: locking B...\n");
    pthread_mutex_lock(&g_mutex_b);   /* ← ждёт, пока BA не отпустит B */

    printf("Thread AB: both locked, done\n");
    pthread_mutex_unlock(&g_mutex_b);
    pthread_mutex_unlock(&g_mutex_a);
    return NULL;
}

static void *ThreadBA(void *arg) {
    (void)arg;
    printf("Thread BA: locking B...\n");
    pthread_mutex_lock(&g_mutex_b);
    printf("Thread BA: B locked, sleeping...\n");
    usleep(100000);

    printf("Thread BA: locking A...\n");
    pthread_mutex_lock(&g_mutex_a);   /* ← ждёт, пока AB не отпустит A */

    printf("Thread BA: both locked, done\n");
    pthread_mutex_unlock(&g_mutex_a);
    pthread_mutex_unlock(&g_mutex_b);
    return NULL;
}

int main(void) {
    pthread_t t1, t2;

    pthread_create(&t1, NULL, ThreadAB, NULL);
    pthread_create(&t2, NULL, ThreadBA, NULL);

    printf("Main: waiting for threads (will hang — this is deadlock)\n");
    fflush(stdout);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    printf("Main: done (this line never prints)\n");
    return 0;
}