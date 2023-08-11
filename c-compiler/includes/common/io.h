#pragma once

#include "common/common.h"

FILE * open_file(const char * filepath, const char options[]);
char * read_file(const char * filepath, size_t * length);
void write_file(const char * filepath, char * buf);
char * get_abs_path(const char * path);
