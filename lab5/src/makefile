CC=gcc
CFLAGS=-Wall -Wextra -pthread

.PHONY: all clean

all: mutex_nolock mutex_lock parallel_factorial deadlock

mutex_nolock: mutex.c
	$(CC) -o mutex_nolock mutex.c $(CFLAGS)

mutex_locked.c: mutex.c
	sed 's|// pthread_mutex_|pthread_mutex_|' mutex.c > mutex_locked.c

mutex_lock: mutex_locked.c
	$(CC) -o mutex_lock mutex_locked.c $(CFLAGS)

parallel_factorial: parallel_factorial.c
	$(CC) -o parallel_factorial parallel_factorial.c $(CFLAGS)

deadlock: deadlock.c
	$(CC) -o deadlock deadlock.c $(CFLAGS)

clean:
	rm -f mutex_nolock mutex_lock mutex_locked.c parallel_factorial deadlock