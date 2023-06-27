#include "AST.h"

struct Ast * init_ast(enum Ast_t type) {
    struct Ast * node = calloc(1, sizeof(struct Ast));

    node->type = type;
    
    node->variables = init_list(sizeof(struct Ast *));

    return node;
}

const char * ast_type_to_str(enum Ast_t type) {
	switch(type) {
        case AST_MODULE: return "AST_MODULE";
        case AST_PACKAGE: return "AST_PACKAGE";
		case AST_FUNCTION: return "AST_FUNCTION";
        case AST_SCOPE: return "AST_SCOPE";
		case AST_ASSIGNMENT: return "AST_ASSIGNMENT";
		case AST_DECLARE: return "AST_DECLARE";
		case AST_VARIABLE: return "AST_VARIABLE";
		case AST_INT: return "AST_INT";
		case AST_STRING: return "AST_STRING";
		case AST_ARRAY: return "AST_ARRAY";
		case AST_ACCESS: return "AST_ACCESS";
		case AST_FOR: return "AST_FOR";
		case AST_RETURN: return "AST_RETURN";
		case AST_IF: return "AST_IF";
		case AST_WHILE: return "AST_WHILE";
		case AST_DO: return "AST_DO";
		case AST_DO_WHILE: return "AST_DO_WHILE";
        case AST_MATCH: return "AST_MATCH";
		case AST_CALL: return "AST_CALL";
		case AST_OP: return "AST_OP";
		case AST_EXPR: return "AST_EXPR";
		case AST_BREAK: return "AST_BREAK";
		case AST_CONTINUE: return "AST_CONTINUE";
		case AST_ROOT: return "AST_ROOT";
		case AST_NOOP: return "AST_NOOP";
	}
	return "UNDEFINED";
}

void free_ast(struct Ast * ast) {
    if (ast->nodes)
	    free_list(ast->nodes);
    
	free(ast->name);
	if (ast->value) {
		free_ast(ast->value);
	}
	free(ast);
}

void print_ast(const char * template, struct Ast * ast) {
	const char * type_str = ast_type_to_str(ast->type);
	const char * scope = ast->scope ? ast_type_to_str(ast->scope->type) : "(NULL)";

	unsigned int nodes_size = 0, 
		    vars = ast->variables->size,
		    v_vars = 0; // ast->v_variables->size;

	if (ast->nodes != NULL) {
		nodes_size = ast->nodes->size;
	}
	
	char * ast_str = format("<name='{s}', type='{s}', [{b}::{b}::{b}], scope='{s}', data='{s}', nodes='{u}', vars='{u}:{u}', s='{s}', int='{i}', p='{b}'>", 
			ast->name,
			type_str,
			ast->left,
			ast->value,
			ast->right,
			scope,
            ast->data_type,
			nodes_size,
			vars,
			v_vars,
			ast->str_value,
			ast->int_value,
			ast->push);

	print(template, ast_str);
	free(ast_str);
}
