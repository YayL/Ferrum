#include "ferrum.h"
#include "common/common.h"

#define LINE_BREAKER "-------------------------\n"

int main(int argc, char ** args){
    if (argc > 1) {
        ferrum_compile(args[1]);
    } else {
        println("Please specifify a source file");
    }
}
