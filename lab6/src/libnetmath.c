#include "libnetmath.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

uint64_t MultModulo(uint64_t a, uint64_t b, uint64_t mod) {
    uint64_t result = 0;
    a = a % mod;
    while (b > 0) {
        if (b % 2 == 1)
            result = (result + a) % mod;
        a = (a * 2) % mod;
        b /= 2;
    }
    return result % mod;
}

bool ConvertStringToUI64(const char *str, uint64_t *val) {
    char *end = NULL;
    errno = 0;                              // сбрасываем, иначе "sticky" ошибки от прошлых вызовов
    unsigned long long i = strtoull(str, &end, 10);
    if (errno == ERANGE) {
        fprintf(stderr, "Out of uint64_t range: %s\n", str);
        return false;
    }
    if (errno != 0) return false;
    if (end == str || *end != '\0') {       // "abc" или "12x" — не число
        fprintf(stderr, "Not a number: %s\n", str);
        return false;
    }
    *val = i;
    return true;
}