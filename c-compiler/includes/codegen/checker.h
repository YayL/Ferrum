#pragma once

#include "codegen/AST.h"
#include "common/hashmap.h"

struct Checker {
    // struct HashMap * types;
    // struct HashMap * traits;
    // struct HashMap * implementations;
};

struct Ast * get_variable(struct Ast * variable);

void checker_check_if(struct Ast * ast);
void checker_check_while(struct Ast * ast);

void checker_check_expr_node(struct Ast * ast);
void checker_check_op(struct Ast * ast);
void checker_check_expression(struct Ast * ast);
void checker_check_variable(struct Ast * ast);

void checker_check_scope(struct Ast * ast);

void checker_check_declaration(struct Ast * ast);
void checker_check_function(struct Ast * ast);
void checker_check_module(struct Ast * ast);

void checker_check(struct Ast * ast);
