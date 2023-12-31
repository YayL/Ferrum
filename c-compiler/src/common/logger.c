#include "common/logger.h"

/* TODO:
 * Change all print calls to logger calls instead
 * Add better color support for logger?
 */

void _assert(char expr, const char * file, const unsigned line, const char * msg) {
    if (!expr) {
        println(YELLOW "{s}" RESET ":" CYAN "{u}" RESET RED "\nASSERTION ERROR" RESET ": " RED "{s}" RESET, file, line, msg);
        exit(1);
    }
}

void logger_log(char * msg, enum LOG_SOURCE source, enum LOG_LEVEL level) {
    if (level > MAX_LOG_LEVEL)
        return;

    const char * fmt;
    const char * color;

    switch (source) {
        case LEXER:
            fmt = "[Lexer]: {2s}" COLOR_RESET_ANSI_SEQUENCE;
            break;
        case PARSER:
            fmt = "[Parser]: {2s}" COLOR_RESET_ANSI_SEQUENCE;
            break;
        case CHECKER:
            fmt = "[Checker]: {2s}" COLOR_RESET_ANSI_SEQUENCE;
            break;
        case IR:
            fmt = "[IR Generator]: {2s}" COLOR_RESET_ANSI_SEQUENCE;
            break;
        case LOGGER:
            fmt = "[Logger]: {2s}" COLOR_RESET_ANSI_SEQUENCE;
            break;
        default:
            logger_log(format("Invalid logger source argument: {i}", source), LOGGER, FATAL);
            exit(1);
    }

    switch (level) {
        case FATAL:
            color = ANSI_START "1;31m";
            break;
        case ERROR:
            color = ANSI_START "1;31;1m";
            break;
        case WARN:
            color = ANSI_START "1;33m";
            break;
        case INFO:
            color = ANSI_START "1;37m";
            break;
        case DEBUG:
            color = ANSI_START "1;32m";
            break;
    }

    println(fmt, color, msg);
}
