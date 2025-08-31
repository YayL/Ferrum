#pragma once
#include "common/ID.h"
#include "common/sourcespan.h"
#include "parser/operators.h"

ID ast_get_scope_id(ID node_id);

void ast_init_node(enum id_type type, void * node_ref);

static inline const char * ast_to_type_str(ID node_id) { return id_type_to_string(node_id.type); }

char * ast_to_string(ID node_id);
void print_ast_tree(ID node_id);
void print_ast(const char * template, ID node_id);

void * get_scope(enum id_type type, ID scope);

struct AST_info {
    ID node_id;
    ID scope_id;
};

typedef struct a_root {
    struct AST_info info;
    char * entry_point;
    khash_t(map_string_to_id) modules;
} a_root;

void print_ast_tree_from_root(a_root root);

typedef struct a_module {
    struct AST_info info;
    char * file_path;
    Arena members;
} a_module;

typedef struct a_import {
    struct AST_info info;
    ID name_id;
    ID module_id;
} a_import;

typedef struct a_function {
    struct AST_info info;
    ID name_id;
    ID arguments_id;
    ID body_id;

    ID param_type;
    ID return_type;

    Arena templates;

    char is_inline;
    char is_checked;
} a_function;

typedef struct a_scope {
    struct AST_info info;
    Arena nodes;
    Arena declarations;
} a_scope;

typedef struct a_declaration {
    struct AST_info info;
    ID expression_id;
    ID name_id;
    char is_const;
} a_declaration;

typedef struct a_expression {
    struct AST_info info;
    Arena children;
    ID type_id;
} a_expression;

typedef struct a_structure {
    struct AST_info info;
    ID name_id;
    Arena templates;
    Arena definitions;
} a_structure;

typedef struct a_enumeration {
    struct AST_info info;
    ID name_id;
    Arena variants;
} a_enumeration;

typedef struct a_trait {
    struct AST_info info;
    ID name_id;
    Arena children;
} a_trait;

typedef struct a_implementation {
    struct AST_info info;
    ID name_id;
    Arena members;
    ID type_id;
} a_implementation;

typedef struct a_operator {
    struct AST_info info;
    struct Operator op;
    ID left_id;
    ID right_id;
    ID type_id;
    struct function_definition {
        ID function_id;
        Arena substitution;
    } definition;
} a_operator;

typedef struct a_symbol {
    struct AST_info info;
    ID node_id;
    Arena name_ids;
} a_symbol;

typedef struct a_variable {
    struct AST_info info;
    ID type_id;
    ID name_id;
    unsigned int reg;
    char is_const;
    char is_declared;
} a_variable;

typedef struct a_literal {
    struct AST_info info;
    ID type_id;
    SourceSpan value;
    enum LITERAL_TYPE {
        LITERAL_NUMBER,
        LITERAL_STRING,
    } literal_type;
} a_literal;

typedef struct a_return_statement {
    struct AST_info info;
    ID expression_id;
} a_return_statement;

typedef struct a_for_statement {
    struct AST_info info;
    ID expression_id;
    ID body_id;
} a_for_statement;

typedef struct a_while_statement {
    struct AST_info info;
    ID expression_id;
    ID body_id;
} a_while_statement;

typedef struct a_if_statement {
    struct AST_info info;
    ID expression_id;
    ID body_id;
    ID next_id;
} a_if_statement;

typedef struct a_match_statement {
    struct AST_info info;
} a_match_statement;

typedef struct a_do_statement {
    struct AST_info info;
} a_do_statement;

typedef struct a_break_statement {
    struct AST_info info;
} a_break_statement;

typedef struct a_continue_statement {
    struct AST_info info;
} a_continue_statement;

typedef struct a_tuple {
    struct AST_info info;
} a_tuple;

typedef struct a_array {
    struct AST_info info;
} a_array;
