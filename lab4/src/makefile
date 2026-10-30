CC=gcc
CFLAGS=-I.

all: parallel_min_max zombie process_memory parallel_sum


parallel_min_max : utils.o find_min_max.o utils.h find_min_max.h
	$(CC) -o parallel_min_max utils.o find_min_max.o parallel_min_max.c $(CFLAGS)


utils.o : utils.h
	$(CC) -o utils.o -c utils.c $(CFLAGS)

find_min_max.o : utils.h find_min_max.h
	$(CC) -o find_min_max.o -c find_min_max.c $(CFLAGS)

zombie : 
	$(CC) -o zombie zombie.c $(CFLAGS)
	
process_memory :
	$(CC) -std=gnu89 -w -o process_memory process_memory.c $(CFLAGS)

parallel_sum: parallel_sum.c libsum.a utils.o
	$(CC) -o parallel_sum parallel_sum.c utils.o $(CFLAGS) $(LDLIBS_SUM)

libsum.a: libsum.o
	ar rcs libsum.a libsum.o

libsum.o: libsum.h
	$(CC) -o libsum.o -c libsum.c $(CFLAGS)

clean :
	rm -f utils.o find_min_max.o parallel_min_max process_memory zombie parallel_sum libsum.o libsum.a
