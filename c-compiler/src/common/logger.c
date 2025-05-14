#include "common/logger.h"

#ifdef BACKTRACE
struct backtrace_state * bt_state = NULL;

void bt_error_callback(void *data, const char *msg, int errnum) {
    println("BT error: {s}", msg);
}

void logger_init(const char * filename) {
    bt_state = backtrace_create_state(filename, 0, bt_error_callback, NULL);
    if (bt_state == NULL) {
        println("logger_init failed");
    }
}

void print_trace() {
    if (bt_state != NULL) {
        backtrace_print(bt_state, 1, stdout);
    }
    // void * array[10];
    // size_t size = backtrace(array, MAX_BACKTRACE_LENGTH);
    // char ** strings = backtrace_symbols(array, size);
    //
    // struct _log_level_rgb color = get_log_level_rgb(LOG_LEVEL_DEBUG);
    // if (strings != NULL) {
    //     for (size_t i = 0; i < size; ++i) {
    //         println("\t" ANSI_RGB_COLOR_FMT "[{i}] {s}", color.R, color.G, color.B, i, strings[i]);
    //     }
    //
    //     free(strings);
    // }
}
#endif
