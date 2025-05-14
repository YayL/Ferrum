#include "common/logger.h"
#include "ferrum.h"
#include "common/common.h"

#define LINE_BREAKER "-------------------------\n"

int main(int argc, char ** args){
    if (argc > 1) {
        #ifdef BACKTRACE
        logger_init(args[0]);
        #endif
        ferrum_compile(args[1]);
    } else {
        println("Please specifify a source file");
    }
}
