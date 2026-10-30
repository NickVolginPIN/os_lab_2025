#ifndef LIBNETMATH_H
#define LIBNETMATH_H

#include <stdint.h>
#include <stdbool.h>

uint64_t MultModulo(uint64_t a, uint64_t b, uint64_t mod);
bool ConvertStringToUI64(const char *str, uint64_t *val);

#endif