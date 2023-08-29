#pragma once

#include "fmt.h"
#include "common/defines.h"
#include "common/list.h"
#include "common/logger.h"

#include <linux/limits.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define ASSERT(expr, msg) _assert(expr, __FILE__, __LINE__, msg)
#define ASSERT1(expr) ASSERT(expr, "")

void _assert(char expr, const char * file, const unsigned line, const char * msg);
