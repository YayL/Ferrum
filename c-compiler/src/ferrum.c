#include "ferrum.h"

#include "common/io.h"
#include "parser/lexer.h"
#include "parser/parser.h"
#include "codegen/checker.h"

void ferrum_compile(char * file_path) {

    struct Ast * ast = init_ast(AST_ROOT);
    
    char * abs_path = get_abs_path(file_path);

    parser_parse(ast, abs_path);
    checker_check(ast);

    print_ast_tree(ast);

    println("\nFinished!");
}

