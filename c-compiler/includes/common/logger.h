#pragma once

#include <execinfo.h>
#include <time.h>
#include <sys/time.h>
#include "common/defines.h"

#ifdef BACKTRACE
#include <backtrace.h>
void bt_error_callback(void *data, const char *msg, int errnum);
void logger_init(const char * filename);
void print_trace();
#endif

#define MAX_BACKTRACE_LENGTH 10

#define ANSI_START "\033["

#define RESET ANSI_START "0m"
#define BOLD ANSI_START "1m"
#define ANSI_RGB_COLOR_FMT ANSI_START "38:2:{i}:{i}:{i}m"
#define RGB_COLOR(R, G, B) ANSI_START "38:2:" #R ":" #G ":" #B "m"

#define RED RGB_COLOR(198, 56, 53) BOLD
#define GREEN RGB_COLOR(81, 172, 56) BOLD
#define YELLOW ANSI_START "33;1m"
#define BLUE RGB_COLOR(19, 96, 178) BOLD
#define MAGENTA ANSI_START "35;1m"
#define CYAN ANSI_START "36;1m"
#define WHITE ANSI_START "37;1m"
#define GREY RGB_COLOR(125, 125, 125)

#ifndef LOG_LEVELS_LIST
#define LOG_LEVELS_LIST(f) \
    f(LOG_LEVEL_FATAL, "Fatal", 200, 50, 40) \
    f(LOG_LEVEL_ERROR, "Error", 230, 80, 70) \
    f(LOG_LEVEL_WARN, "Warn", 222, 172, 22) \
    f(LOG_LEVEL_INFO, "Info", 200, 200, 200) \
    f(LOG_LEVEL_DEBUG, "Debug", 160, 160, 160)

#define GET_LOG_LEVEL_ENUM_NAME(NAME, ...) NAME,
enum log_levels_t {
    LOG_LEVELS_LIST(GET_LOG_LEVEL_ENUM_NAME)
};

#define GENERATE_LOG_LEVEL_TO_STR(LEVEL, STR, ...) case LEVEL: return STR;
static inline const char * get_log_level_str(enum log_levels_t level) { \
    switch (level) { \
        LOG_LEVELS_LIST(GENERATE_LOG_LEVEL_TO_STR) \
        default: return "UNDEFINED"; \
    } \
}

struct _log_level_rgb {
    int R, G, B;
};

#define GENERATE_LOG_LEVEL_RGB(LEVEL, STR, _R, _G, _B) case LEVEL: return (struct _log_level_rgb) {.R=_R, .G=_G, .B=_B};
static inline const struct _log_level_rgb get_log_level_rgb(enum log_levels_t level) { \
        switch (level) { \
            LOG_LEVELS_LIST(GENERATE_LOG_LEVEL_RGB) \
            default: return (struct _log_level_rgb) {.R=0, .G=0, .B=0}; \
        } \
    }

#endif

static inline int get_milliseconds() {
   struct timeval tv;
   gettimeofday(&tv, NULL);
   return tv.tv_usec / 1000;
}

#define CSTR_LEN(str) (sizeof(str) - 1)

static inline struct tm get_time() {
   time_t time_now;
   struct tm time_info;

   time(&time_now);
#ifdef _WIN32
   localtime_s(&time_info, &time_now);
#else
   localtime_r(&time_now, &time_info);
#endif

   return time_info;
}

#define TO_STR(x) #x
#define TO_STR_VALUE(x) TO_STR(x)

#define LOGGER_LOG(LEVEL, FMT, ...) { \
   struct _log_level_rgb color = get_log_level_rgb(LEVEL); \
   println(ANSI_START "38:2:{i}:{i}:{i}m[" __FILE_NAME__ ":" TO_STR_VALUE(__LINE__) "] [{s}] " FMT RESET, color.R, color.G, color.B, get_log_level_str(LEVEL), ##__VA_ARGS__); \
}

// Change end of this macro the wanted min log level
#define MIN_LOG_LEVEL_INFO

#define INFO(FMT, ...)
#define DEBUG(FMT, ...)
#define WARN(FMT, ...)
#define ERROR(FMT, ...)
#define FATAL(FMT, ...) LOGGER_LOG(LOG_LEVEL_FATAL, FMT, ##__VA_ARGS__); print_trace(); exit(1)

#ifndef MIN_LOG_LEVEL_FATAL
#undef ERROR
#define ERROR(FMT, ...) LOGGER_LOG(LOG_LEVEL_ERROR, FMT, ##__VA_ARGS__)
#ifndef MIN_LOG_LEVEL_ERROR
#undef WARN
#define WARN(FMT, ...) LOGGER_LOG(LOG_LEVEL_WARN, FMT, ##__VA_ARGS__)
#ifndef MIN_LOG_LEVEL_WARN
#undef DEBUG
#define DEBUG(FMT, ...) LOGGER_LOG(LOG_LEVEL_DEBUG, FMT, ##__VA_ARGS__)
#ifndef MIN_LOG_LEVEL_DEBUG
#undef INFO
#define INFO(FMT, ...) LOGGER_LOG(LOG_LEVEL_INFO, FMT, ##__VA_ARGS__)
#endif
#endif
#endif
#endif

#define ASSERT(EXPR, FMT, ...) if (!(EXPR)) { FATAL(FMT, ##__VA_ARGS__); }
#define ASSERT1(EXPR) ASSERT(EXPR, "Assertion failed: " #EXPR)
