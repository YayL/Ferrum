#pragma once

#include <limits.h>

#define log2i(x) ((sizeof(x) * CHAR_BIT) - __builtin_clz(x))
