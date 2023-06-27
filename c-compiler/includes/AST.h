#pragma once

#include "list.h"

struct Ast {

	enum Ast_t {
		AST_ROOT,
		AST_MODULE,
        AST_PACKAGE,
		AST_FUNCTION,
        AST_SCOPE,
		AST_ASSIGNMENT,
		AST_DECLARE,
		AST_VARIABLE,
		AST_ARRAY,
		AST_INT,
		AST_STRING,
		AST_ACCESS,
		AST_CALL,
		AST_OP,
		AST_EXPR,
		AST_RETURN,
		AST_FOR,
		AST_IF,
		AST_WHILE,
		AST_DO,
		AST_DO_WHILE,
        AST_MATCH,
		AST_BREAK,
		AST_CONTINUE,
		AST_NOOP,
	} type;
	struct Ast * scope;
	struct Ast * left;
	struct Ast * value;
	struct Ast * right;
	struct List * nodes;
	struct List * variables;
    struct Operator * operator;
	int int_value;
	char push;
	char * str_value;
	const char * name;
    char * data_type;
	struct Ast * (*f_ptr)();

};

struct Ast * init_ast(enum Ast_t type);
void free_ast(struct Ast * node);
void set_ast(struct Ast * dest, struct Ast * src);
const char * ast_type_to_str(enum Ast_t type);
void print_ast(const char * template, struct Ast * node);
