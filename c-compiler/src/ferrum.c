#include "ferrum.h"

#include "common/io.h"
#include "common/logger.h"
#include "fmt.h"
#include "parser/parser.h"
#include "codegen/checker.h"
#include "codegen/gen.h"
#include "tables/interner.h"

#include <sys/time.h>

#define LINE_BREAKER "-------------------------------------"

struct timeval t_start, t_stop;

void start_timer() {
    gettimeofday(&t_start, NULL);
}

unsigned long stop_timer() {
    gettimeofday(&t_stop, NULL);
    return 1000000 * (t_stop.tv_sec - t_start.tv_sec) + (t_stop.tv_usec - t_start.tv_usec);
}

void ferrum_compile(char * file_path) {


    // INFO("Hello, Info!");
    // DEBUG("Hello, Debug!");
    // WARN("Hello, Warn!");
    // ERROR("Hello, Error!");
    // FATAL("Hello, Fatal!");

    // return;

    struct AST * ast = init_ast(AST_ROOT, NULL);
    interner_init();
    
    char * abs_path = get_abs_path(file_path),
         * parser_time,
         * checker_time,
         * gen_time,
         * optimization_time;

    long time, total = 0;

    start_timer();
    parser_parse(ast, abs_path);
    time = stop_timer();
    total += time;
    asprintf(&parser_time, "Time for parser:\t%.3fms", (double)time / 1000);

    print_ast_tree(ast);

    start_timer();
    checker_check(ast);
    time = stop_timer();
    total += time;
    asprintf(&checker_time, "Time for checker:\t%.3fms", (double)time / 1000);

    print_ast_tree(ast);

//     const char * OUTPUT_PATH = "./build/ferrum.ll";
//     FILE * fp = open_file(get_abs_path(OUTPUT_PATH), "w");
//     start_timer();
//     gen(fp, ast);
//     time = stop_timer();
//     total += time;
//     asprintf(&gen_time, "Time for generator:\t%.3fms", (double)time / 1000);
//     fclose(fp);
//
//     start_timer();
// #ifdef OUTPUT_OPTIMISED_LLVM
//     system("clang ./build/ferrum.ll -emit-llvm -S -c -O3 -o ferrum.ll && clang ferrum.ll");
// #else
//     system("clang ./build/ferrum.ll -O3");
// #endif
//     time = stop_timer();
//     total += time;
//     asprintf(&optimization_time, "Time for optimizer:\t%.3fms", (double)time / 1000);

    puts(LINE_BREAKER);
    puts(parser_time);
    puts(checker_time);
    // puts(gen_time);
    // puts(optimization_time);
    puts(LINE_BREAKER);
    printf("Total: %.3fms\n", (double)total / 1000);
}

