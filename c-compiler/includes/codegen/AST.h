#pragma once

#include "common/list.h"

enum AST_type {
    AST_ROOT,
    AST_MODULE,
    AST_FUNCTION,
    AST_SCOPE,
    AST_DECLARATION,
    AST_EXPR,
    AST_OP,
    AST_VARIABLE,
    AST_TYPE,
    AST_LITERAL,
    AST_TUPLE,
    AST_ARRAY,
    AST_FOR,
    AST_RETURN,
    AST_WHILE,
    AST_IF,
    AST_BREAK,
    AST_CONTINUE
};

struct Ast {
    enum Ast_type type;
    struct Ast * scope;
    void * value;
};

struct AST_ROOT {
    struct List * modules;
};

struct AST_MODULE {
    char * path;
    struct List * variables;
    struct List * functions;
    // struct List * traits;
    // struct List * implementations;
};

struct AST_FUNCTION {
    char * name;
    struct AST_SCOPE * scope;
    struct AST_TYPE * type;
    struct List * arguments;
};

struct AST_SCOPE {
    struct List * variables;
    struct List * nodes;
};

struct AST_DECLARATION {
    struct List * expressions;
};

struct AST_EXPR {
    struct Ast * child;
};

struct AST_OP {
    struct Operator * op;
    struct Ast * left;
    struct Ast * right;
};

struct AST_VARIABLE {
    char * name;
    struct AST_TYPE * type;
};

struct AST_TYPE {
    char * name;
};

struct AST_LITERAL {
    enum LITERAL_TYPE {
        INTEGER,
        STRING,
    } type;
    char * value;
};

struct AST_TUPLE {
    struct List * elements;
};

struct AST_RETURN {
    struct AST_EXPR * expression;
};

struct AST_FOR {
    struct AST_EXPR * expression;
    struct AST_SCOPE * scope;
};

struct AST_WHILE {
    struct AST_EXPR * expression;
    struct AST_SCOPE * scope;
};

struct AST_IF {
    struct AST_EXPR * expression;
    struct AST_SCOPE * scope;
    struct AST_IF * next;
};


void * init_ast(enum Ast_type type, char return_value);
void free_ast(struct Ast * node);
void set_ast(struct Ast * dest, struct Ast * src);
const char * ast_type_to_str(enum Ast_type type);
void print_ast(const char * template, struct Ast * node);
