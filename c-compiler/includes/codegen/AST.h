#pragma once
#include "checker/typing.h"
#include "common/sourcespan.h"
#include "parser/operators.h"
#include "parser/types.h"

#include "common/list.h"
#include "common/hashmap.h"
#include "common/string.h"

#include "common/macro.h"
#include "tables/symbol.h"

#define DEREF_AST(ast) ((struct AST *) ast)->value
#define get_type_str(type) (type != NULL ? type_to_str(*type) : "void")

#ifndef AST_FOREACH
#define AST_FOREACH(f) \
    f(AST_ROOT, "Root", a_root) \
    f(AST_MODULE, "Module", a_module) \
    f(AST_FUNCTION, "Function", a_function) \
    f(AST_SCOPE, "Scope", a_scope) \
    f(AST_DECLARATION, "Declaration", a_declaration) \
    f(AST_EXPR, "Expression", a_expr) \
    f(AST_OP, "Operator", a_op) \
    f(AST_CALL, "Call", void) \
    f(AST_VARIABLE, "Variable", a_variable) \
    f(AST_LITERAL, "Literal", a_literal) \
    f(AST_TUPLE, "Tuple", void) \
    f(AST_ARRAY, "Array", void) \
    f(AST_FOR, "For", a_for_statement) \
    f(AST_RETURN, "Return", a_return) \
    f(AST_WHILE, "While", a_while_statement) \
    f(AST_IF, "If", a_if_statement) \
    f(AST_MATCH, "Match", void) \
    f(AST_DO, "Do", void) \
    f(AST_BREAK, "Break", void) \
    f(AST_CONTINUE, "Continue", void) \
    f(AST_STRUCT, "Struct", a_enum) \
    f(AST_ENUM, "Enum", a_enum) \
    f(AST_TRAIT, "Trait", a_trait) \
    f(AST_IMPL, "Implementation", a_impl) 
#endif

#define GEN_ENUM_EL(el_name, ...) el_name,
enum AST_type {
    AST_FOREACH(GEN_ENUM_EL)
};

typedef struct a_root {
    struct List * modules;
    struct hashmap * markers;
} a_root;

typedef struct a_module {
    char * path;
    struct symbol_table symbol_table;

    struct hashmap * symbols;
    struct List * variables;
    struct List * functions;
    struct hashmap * functions_map;
    struct List * structures;
    struct List * traits;
    struct List * impls;
} a_module;

typedef struct a_function {
    unsigned int interner_id;
    struct AST * arguments;
    struct AST * body;

    Type * param_type;
    Type * return_type;

    struct List * parsed_templates;
    struct hashmap * template_types;

    char is_inline;
    char is_checked;
} a_function;

typedef struct a_scope {
    struct List * nodes;
    struct symbol_table symbol_table;
} a_scope;

typedef struct a_declaration {
    struct AST * expression;
    struct AST * variable;
    char is_const;
} a_declaration;

typedef struct a_expr {
    struct List * children;
    Type * type;
} a_expr;

typedef struct a_struct {
    unsigned int interner_id;
    struct List * generics; // struct NAME<GEN1, GEN2>
    struct List * variables;
    struct List * functions;
    Type * type;
} a_struct;

typedef struct a_enum {
    char * name;
    struct List * variants;
} a_enum;

typedef struct a_trait {
    unsigned int interner_id;
    struct List * implementers;
    struct List * children;
} a_trait;

typedef struct a_impl {
    unsigned int interner_id;
    Type * type;
    struct List * members;
} a_impl;

typedef struct a_op {
    struct Operator * op;
    struct AST * left;
    struct AST * right;
    Type * type;
    struct function_definition {
        struct AST * function;
        Arena substitution;
    } definition;
} a_op;

typedef struct a_variable {
    unsigned int interner_id;
    Type * type;
    unsigned int reg;
    char is_const;
    char is_declared;
} a_variable;

typedef struct a_literal {
    Type * type;
    SourceSpan value;
    enum LITERAL_TYPE {
        LITERAL_NUMBER,
        LITERAL_STRING,
    } literal_type;
} a_literal;

typedef struct a_return {
    struct AST * expression;
} a_return;

typedef struct a_for_statement {
    struct AST * expression;
    struct AST * body;
} a_for_statement;

typedef struct a_while_statement {
    struct AST * expression;
    struct AST * body;
} a_while_statement;

typedef struct a_if_statement {
    struct AST * expression;
    struct AST * body;
    struct a_if_statement * next;
} a_if_statement;

struct AST {
    enum AST_type type;
    struct AST * scope;
    union AST_union {
        a_root root;
        a_module module;
        a_function function;
        a_scope scope;
        a_declaration declaration;
        a_expr expression;
        a_struct structure;
        a_enum enumeration;
        a_trait trait;
        a_impl implementation;
        a_op operator;
        a_variable variable;
        a_literal literal;
        a_return return_statement;
        a_for_statement for_statement;
        a_while_statement while_statement;
        a_if_statement if_statement;
    } value;
};

struct AST * init_ast(enum AST_type type, struct AST * scope);
union AST_union init_ast_value(enum AST_type type);

void free_ast(struct AST * node);
void set_ast(struct AST * dest, struct AST * src);

const char * ast_type_to_str_ast(struct AST * ast);
const char * ast_type_to_str(enum AST_type type);
const size_t ast_union_size(enum AST_type type);

char * ast_to_string(struct AST * ast);
void print_ast_tree(struct AST * node);
void print_ast(const char * template, struct AST * node);

struct AST * get_scope(enum AST_type type, struct AST * scope);
