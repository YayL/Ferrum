#pragma once

#include <stdint.h>
#include <limits.h>

#define log2i(x) ((sizeof(x) * CHAR_BIT) - __builtin_clz(x))

// boost's hash_combine
static inline uint64_t hash_combine(uint64_t h, uint64_t v) {
    h ^= v + 0x9e3779b9 + (h << 6) + (h >> 2);
    return h;
}
