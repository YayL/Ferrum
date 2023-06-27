#include <time.h>

#include "ferrum.h"
#include "common.h"

int main(int argc, char ** args) {
    
    clock_t start = clock(), end;
    if (argc > 1) {
        ferrum_compile_file(args[1]);
    } else {
        println("Please specifify a source file");
    }
    end = clock();
    println("Time to compile: {lu}", end - start);
}
