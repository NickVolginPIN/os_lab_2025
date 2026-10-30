#ifndef LIBSUM_H
#define LIBSUM_H

struct SumArgs {
    int *array;
    int begin;
    int end;
};

int Sum(const struct SumArgs *args);

#endif