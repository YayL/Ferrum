#pragma once

#define ALLOC(dest) dest = malloc(sizeof(typeof(*dest)))
