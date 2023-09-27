#include "ferrum.h"

#include "common/io.h"
#include "parser/lexer.h"
#include "parser/parser.h"
#include "codegen/checker.h"
#include "codegen/gen.h"

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

    struct Ast * ast = init_ast(AST_ROOT, NULL);
    
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
    asprintf(&parser_time, LINE_BREAKER "\nTime for parser:\t%.3fms", (double)time / 1000);

    /* start_timer(); */
    /* checker_check(ast); */
    /* time = stop_timer(); */
    /* total += time; */
    /* asprintf(&checker_time, LINE_BREAKER "\nTime for checker:\t%.3fms", (double)time / 1000); */

    print_ast_tree(ast);

    /* start_timer(); */
    /* gen(ast); */
    /* time = stop_timer(); */
    /* total += time; */
    /* asprintf(&gen_time, LINE_BREAKER "\nTime for generator:\t%.3fms", (double)time / 1000); */

    /* start_timer(); */
    /* //system("clang ./build/ferrum.ll -emit-llvm -S -c -O3 -o ferrum.ll && llc ferrum.ll"); */
    /* time = stop_timer(); */
    /* total += time; */
    /* asprintf(&optimization_time, LINE_BREAKER "\nTime for optimizer:\t%.3fms\n" LINE_BREAKER, (double)time / 1000); */

    puts(parser_time);
    //puts(checker_time);
    //puts(gen_time);
    //puts(optimization_time);
    printf("Total: %.3fms\n", (double)total / 1000);
}

