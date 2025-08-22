#pragma once
#include "common/sourcespan.h"
#include "parser/operators.h"
#include "parser/types.h"

#include "common/list.h"
#define DEREF_AST(ast) ((struct AST *) ast)->value
#define get_type_str(type) (type != NULL ? type_to_str(*type) : "void")

#ifndef AST_FOREACH
#define AST_FOREACH(f) \
    f(AST_ROOT, "Root", root) \
    f(AST_MODULE, "Module", module) \
    f(AST_IMPORT, "Import", import) \
    f(AST_FUNCTION, "Function", function) \
    f(AST_SCOPE, "Scope", scope) \
    f(AST_DECLARATION, "Declaration", declaration) \
    f(AST_EXPR, "Expression", expression) \
    f(AST_OP, "Operator", operator) \
    f(AST_SYMBOL, "Symbol", symbol) \
    f(AST_VARIABLE, "Variable", variable) \
    f(AST_LITERAL, "Literal", literal) \
    f(AST_TUPLE, "Tuple", tuple) \
    f(AST_ARRAY, "Array", array) \
    f(AST_FOR, "For", for_statement) \
    f(AST_RETURN, "Return", return_statement) \
    f(AST_WHILE, "While", while_statement) \
    f(AST_IF, "If", if_statement) \
    f(AST_MATCH, "Match", match_statement) \
    f(AST_DO, "Do", do_statement) \
    f(AST_BREAK, "Break", break_statement) \
    f(AST_CONTINUE, "Continue", continue_statement) \
    f(AST_STRUCT, "Struct", structure) \
    f(AST_ENUM, "Enum", enumeration) \
    f(AST_TRAIT, "Trait", trait) \
    f(AST_IMPL, "Implementation", implementation) 
#endif

#define GEN_ENUM_EL(el_name, ...) el_name,
enum AST_type {
    AST_FOREACH(GEN_ENUM_EL)
};

KHASH_MAP_INIT_STR(modules_hm, struct AST *);

typedef struct a_root {
    char * entry_point;
    khash_t(modules_hm) modules;
} a_root;

typedef struct a_module {
    char * file_path;
    Arena members;
} a_module;

typedef struct a_import {
    ID name_id;
    ID module_id;
} a_import;

typedef struct a_function {
    ID arguments_id;
    ID body_id;

    Type param_type;
    Type return_type;

    Arena templates;

    ID name_id;

    char is_inline;
    char is_checked;
} a_function;

typedef struct a_scope {
    Arena nodes;
    Arena declarations;
} a_scope;

typedef struct a_declaration {
    ID expression_id;
    ID name_id;
    char is_const;
} a_declaration;

typedef struct a_expression {
    Arena children;
    Type type;
} a_expression;

typedef struct a_structure {
    ID name_id;
    Arena templates;
    Arena definitions;
} a_structure;

typedef struct a_enumeration {
    ID name_id;
    Arena variants;
} a_enumeration;

typedef struct a_trait {
    ID name_id;
    Arena children;
} a_trait;

typedef struct a_implementation {
    ID name_id;
    Arena members;
    Type type;
} a_implementation;

typedef struct a_operator {
    struct Operator * op;
    ID left_id;
    ID right_id;
    Type type;
    struct function_definition {
        ID function_id;
        Arena substitution;
    } definition;
} a_operator;

typedef struct a_symbol {
    struct AST * node;
    Arena name_ids;
} a_symbol;

typedef struct a_variable {
    Type type;
    ID name_id;
    unsigned int reg;
    char is_const;
    char is_declared;
} a_variable;

typedef struct a_literal {
    Type type;
    SourceSpan value;
    enum LITERAL_TYPE {
        LITERAL_NUMBER,
        LITERAL_STRING,
    } literal_type;
} a_literal;

typedef struct a_return_statement {
    ID expression_id;
} a_return_statement;

typedef struct a_for_statement {
    ID expression_id;
    ID body_id;
} a_for_statement;

typedef struct a_while_statement {
    ID expression_id;
    ID body_id;
} a_while_statement;

typedef struct a_if_statement {
    ID expression_id;
    ID body_id;
    ID next_id;
} a_if_statement;

typedef struct a_match_statement {} a_match_statement;
typedef struct a_do_statement {} a_do_statement;
typedef struct a_break_statement {} a_break_statement;
typedef struct a_continue_statement {} a_continue_statement;

typedef struct a_tuple { } a_tuple;
typedef struct a_array { } a_array;

#define GEN_AST_UNION(ENUM, STR, VALUE) a_##VALUE VALUE;
struct AST {
    ID node_id;
    ID scope_id;
    enum AST_type type;
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
