#pragma once

#include "codegen/AST.h"
#include "common/hashmap.h"
#include "codegen/functions.h"

struct Checker {
    // struct HashMap * types;
    // struct HashMap * traits;
    // struct HashMap * implementations;
};

struct AST * get_symbol(char * const name, struct AST const * scope);
struct AST * get_variable(struct AST * variable);

void add_marker(struct AST * marker, const char * name);
struct AST * get_marker(struct AST * ast, const char * name);

void checker_check_if(struct AST * ast);
void checker_check_while(struct AST * ast);

Type checker_check_expr_node(struct AST * ast);
Type checker_check_op(struct AST * ast);
Type checker_check_expression(struct AST * ast);
Type checker_check_variable(struct AST * ast);

void checker_check_scope(struct AST * ast);

void checker_check_declaration(struct AST * ast);
void checker_check_function(struct AST * ast);
struct AST * checker_check_module(struct AST * ast);

void checker_check(struct AST * ast);
