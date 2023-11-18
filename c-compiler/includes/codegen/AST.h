#pragma once
#include "parser/operators.h"
#include "parser/types.h"

#include "common/list.h"
#include "common/hashmap.h"
#include "common/string.h"

#define DEREF_AST(ast) ((struct Ast *) ast)->value
#define get_type_str(ast) (ast != NULL ? type_to_str((a_type *) ast->value) : "void")

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
    AST_CONTINUE,
    AST_STRUCT,
    AST_ENUM,
    AST_TRAIT,
    AST_IMPL
};

typedef Type a_type;

struct Ast {
    enum AST_type type;
    struct Ast * scope;
    void * value;
};

typedef struct a_root {
    struct List * modules;
    struct HashMap * markers;
} a_root;

typedef struct a_module {
    char * path;
    struct HashMap * symbols;
    struct List * variables;
    struct List * functions;
    struct List * structures;
    struct List * traits;
    struct List * impls;
} a_module;

typedef struct a_function {
    char * name;
    struct Ast * body;
    struct Ast * param_type;
    struct Ast * return_type;
    struct Ast * arguments; // expression node
    char is_inline;
} a_function;

typedef struct a_scope {
    struct List * variables;
    struct List * nodes;
} a_scope;

typedef struct a_declaration {
    struct Ast * expression;
    struct Ast * variable;
    char is_const;
} a_declaration;

typedef struct a_expr {
    struct List * children;
    struct Ast * type;
} a_expr;

typedef struct a_struct {
    char * name;
    struct List * generics; // struct NAME<GEN1, GEN2>
    struct List * variables;
    struct List * functions;
    struct Ast * type;
} a_struct;

typedef struct a_enum {
    char * name;
    struct List * variants;
} a_enum;

typedef struct a_trait {
    char * name;
    struct List * impls;
    struct List * children;
} a_trait;

typedef struct a_impl {
    char * name;
    struct Ast * type;
    struct List * members;
} a_impl;

typedef struct a_op {
    struct Operator * op;
    struct Ast * left;
    struct Ast * right;
    struct Ast * type;
    struct Ast * definition;
} a_op;

typedef struct a_variable {
    char * name;
    struct Ast * type;
    unsigned int reg;
    char is_const;
    char is_declared;
} a_variable;

typedef struct a_literal {
    struct Ast * type;
    char * value;
    enum LITERAL_TYPE {
        LITERAL_NUMBER,
        LITERAL_STRING,
    } literal_type;
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


struct Ast * init_ast(enum AST_type type, struct Ast * scope);
void * init_ast_of_type(enum AST_type type);

void free_ast(struct Ast * node);
void set_ast(struct Ast * dest, struct Ast * src);

const char * ast_type_to_str_ast(struct Ast * ast);
const char * ast_type_to_str(enum AST_type type);
struct Ast * ast_get_type_of(struct Ast * ast);

void print_ast_tree(struct Ast * node);
void print_ast(const char * template, struct Ast * node);
