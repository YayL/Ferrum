#include "codegen/checker.h"

void checker_check_declaration(struct Checker * checker, struct Ast * node) {

}

void checker_check_function(struct Checker * checker, struct Ast * node) {

}

void checker_check_module(struct Checker * checker, struct Ast * module) {
    const int size = module->nodes->size;
    struct Ast * node;

    for (int i = 0; i < size; ++i) {
        node = list_at(module->nodes, i);
        switch (node->type) {
            case AST_DECLARE:
                checker_check_declaration(checker, node);
                break;
            case AST_FUNCTION:
                checker_check_function(checker, node);
                break;
            default:
                print_ast("[Error] Not implemented\n{s}", node);
                exit(1);
        }
    }

}

struct Checker * checker_check(struct Ast * root) {

    struct Checker * checker = malloc(sizeof(struct Checker));
    checker->variables = new_HashMap(8);
    checker->variables = new_HashMap(8);

    for (int i = 0; i < root->nodes->size; ++i) {
        checker_check_module(checker, list_at(root->nodes, i));
    }

    return checker;
}
