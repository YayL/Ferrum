#pragma once
#include "parser/operators.h"

#include "common/list.h"
#include "common/string.h"

enum AST_type {
    AST_ROOT,
    AST_MODULE,
    AST_FUNCTION,
    AST_SCOPE,
    AST_DECLARATION,
    AST_EXPR,
    AST_OP,
    AST_CALL,
    AST_VARIABLE,
    AST_TYPE,
    AST_LITERAL,
    AST_TUPLE,
    AST_ARRAY,
    AST_FOR,
    AST_RETURN,
    AST_WHILE,
    AST_IF,
    AST_MATCH,
    AST_DO,
    AST_BREAK,
    AST_CONTINUE
};

struct Ast {
    enum AST_type type;
    struct Ast * scope;
    void * value;
};

typedef struct a_root {
    struct List * modules;
} a_root;

typedef struct a_module {
    char * path;
    struct List * variables;
    struct List * functions;
    // struct List * traits;
    // struct List * implementations;
} a_module;

typedef struct a_function {
    char * name;
    struct Ast * body;
    // struct Ast * type; might implement at some point?
    char * type;
    struct List * arguments;
} a_function;

typedef struct a_scope {
    struct List * variables;
    struct List * nodes;
} a_scope;

typedef struct a_declaration {
    struct Ast * expression;
} a_declaration;

typedef struct a_expr {
    struct List * children;
} a_expr;

typedef struct a_op {
    struct Operator * op;
    struct Ast * left;
    struct Ast * right;
} a_op;

typedef struct a_variable {
    char * name;
    // struct Ast * type; same as function here
    char * type;
} a_variable;

typedef struct a_type {
    char * name;
} a_type;

typedef struct a_literal {
    enum LITERAL_TYPE {
        NUMBER,
        STRING,
    } type;
    char * value;
} a_literal;

typedef struct a_return {
    struct Ast * expression;
} a_return;

typedef struct a_for_statement {
    struct Ast * expression;
    struct Ast * body;
} a_for_statement;

typedef struct a_while_statement {
    struct Ast * expression;
    struct Ast * body;
} a_while_statement;

typedef struct a_if_statement {
    struct Ast * expression;
    struct Ast * body;
    struct a_if_statement * next;
} a_if_statement;


struct Ast * init_ast(enum AST_type type);
void * init_ast_of_type(enum AST_type type);

void free_ast(struct Ast * node);
void set_ast(struct Ast * dest, struct Ast * src);

const char * ast_type_to_str_ast(struct Ast * ast);
const char * ast_type_to_str(enum AST_type type);

void print_ast_tree(struct Ast * node);
void print_ast(const char * template, struct Ast * node);
