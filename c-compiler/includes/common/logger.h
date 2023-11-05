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
    CHECKER,
    IR,
    LOGGER
};

#define MAX_LOG_LEVEL ((enum LOG_LEVEL) DEBUG)
#define ANSI_START "\033["
#define COLOR_RESET_ANSI_SEQUENCE ANSI_START "0m"

#define RESET ANSI_START "0m"
#define BOLD ANSI_START "1m"
#define RED ANSI_START "38:2:198:56:53m" BOLD
#define GREEN ANSI_START "38:2:81:172:56m" BOLD
#define YELLOW ANSI_START "33;1m"
#define BLUE ANSI_START "38:2:19:96:178m" BOLD
#define MAGENTA ANSI_START "35;1m"
#define CYAN ANSI_START "36;1m"
#define WHITE ANSI_START "37;1m"
#define GREY ANSI_START "38:2:125:125:125m"

void logger_log(char * msg, enum LOG_SOURCE source, enum LOG_LEVEL level);
