#pragma once

#include "common/common.h"

enum LOG_LEVEL {
    FATAL,
    ERROR,
    WARN,
    INFO,
    DEBUG
};

enum LOG_SOURCE {
    LEXER,
    PARSER,
    IR,
    LOGGER
};

#define MAX_LOG_LEVEL ((enum LOG_LEVEL) DEBUG)
#define ANSI_START "\033["
#define COLOR_RESET_ANSI_SEQUENCE ANSI_START "0m"

void logger_log(char * msg, enum LOG_SOURCE source, enum LOG_LEVEL level);
