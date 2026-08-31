#ifndef SPATTER_COMMON_H
#define SPATTER_COMMON_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#define SPATTER_UNUSED(x) ((void)(x))
#define SPATTER_MAX(a, b) (((a) > (b)) ? (a) : (b))
#define SPATTER_MIN(a, b) (((a) < (b)) ? (a) : (b))
#define SPATTER_EPSILON 1e-12
#define SPATTER_NEG_INF -1e9

static inline void* spatter_alloc(size_t size)
{
    void* ptr = malloc(size);
    if (!ptr && size > 0) {
        fprintf(stderr, "[FATAL] Memory allocation failed for %zu bytes\n", size);
        exit(EXIT_FAILURE);
    }
    return ptr;
}

static inline void* spatter_calloc(size_t count, size_t size)
{
    void* ptr = calloc(count, size);
    if (!ptr && count > 0 && size > 0) {
        fprintf(stderr, "[FATAL] Zeroed memory allocation failed for %zu items of %zu bytes\n", count, size);
        exit(EXIT_FAILURE);
    }
    return ptr;
}

static inline void spatter_free(void* ptr)
{
    free(ptr);
}

#endif /* SPATTER_COMMON_H */
