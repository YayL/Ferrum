#include "codegen/checker.h"

void checker_check_declaration(struct Checker * checker, struct Ast * node) {

}

void checker_check_function(struct Checker * checker, struct Ast * node) {

}

void checker_check_module(struct Checker * checker, struct Ast * ast) {
    a_module * module = ast->value;
    struct Ast * node;
    int size = module->variables->size;

    for (int i = 0; i < size; ++i) {
        checker_check_declaration(checker, list_at(module->variables, i));
    }
    
    size = module->functions->size;
    for (int i = 0; i < size; ++i) {
        checker_check_function(checker, list_at(module->functions, i));
    }
}

struct Checker * checker_check(struct Ast * ast) {

    struct Checker * checker = malloc(sizeof(struct Checker));
    checker->variables = new_HashMap(8);
    checker->variables = new_HashMap(8);

    a_root * root = ast->value;

    for (int i = 0; i < root->modules->size; ++i) {
        checker_check_module(checker, list_at(root->modules, i));
    }

    return checker;
}
