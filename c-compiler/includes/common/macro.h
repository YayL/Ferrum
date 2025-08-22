#pragma once

#define ALLOC(dest) if (dest == NULL)  { dest = malloc(sizeof(typeof(*dest))); }
