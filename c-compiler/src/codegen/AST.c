#include "codegen/AST.h"

void * init_ast(enum Ast_type type, char return_value) {
    void * value;

    switch (type) {
        case AST_ROOT:
        {
            struct AST_ROOT * root = malloc(sizeof(struct AST_ROOT));
            value = root;
            root->modules = init_list(sizeof(struct AST_MODULE *));
            break;
        }
        case AST_MODULE:
        {
            struct AST_MODULE * module = malloc(sizeof(struct AST_MODULE));
            value = module;
            module->functions = init_list(sizeof(struct AST_FUNCTION *));
            module->variables = init_list(sizeof(struct AST_VARIABLE *));
            break;
        }
        case AST_FUNCTION:
        {
            struct AST_FUNCTION * function = malloc(sizeof(struct AST_FUNCTION));
            value = function;
            function->scope = init_ast(AST_SCOPE, 1);
            function->type = init_
            break;
        }



    }

    if (return_value)
        return value;
    
    struct Ast * node = malloc(sizeof(struct Ast));
    node->type = type;
    node->value = value;

    return node;
}

const char * ast_type_to_str(enum Ast_type type) {
	switch(type) {
        case AST_MODULE: return "Module";
		case AST_FUNCTION: return "Function";
        case AST_SCOPE: return "Scope";
        case AST_TYPE: return "Type";
        case AST_DECLARATION: return "Declaration";
		case AST_VARIABLE: return "Variable";
        case AST_LITERAL: return "Literal";
        case AST_TUPLE: return "Tuple";
		case AST_FOR: return "For";
		case AST_RETURN: return "Return";
		case AST_IF: return "If";
		case AST_WHILE: return "While";
		case AST_OP: return "Operator";
		case AST_EXPR: return "Expression";
		case AST_BREAK: return "Break";
        case AST_CONTINUE: return "Continue";
		case AST_ROOT: return "Root";
	}
	return "UNDEFINED";
}

void free_ast(struct Ast * ast) {
    if (ast->name != NULL)
	    free(ast->name);
    if (ast->nodes != NULL) {
        free(ast->nodes->items);
        free(ast->nodes);
    }
	free(ast);
}

void print_ast(const char * template, struct Ast * ast) {
	const char * type_str = ast_type_to_str(ast->type);
	const char * scope = ast->scope ? ast_type_to_str(ast->scope->type) : "(NULL)";

	unsigned int nodes_size = 0;

	if (ast->nodes != NULL) {
		nodes_size = ast->nodes->size;
	}
	
	char * ast_str = format("<name='{s}', type='{s}', [{b}::{b}::{b}], scope='{s}', data='{s}', nodes='{u}', s='{s}', int='{i}', p='{b}'>", 
			ast->name,
			type_str,
			ast->left,
			ast->value,
			ast->right,
			scope,
            ast->data_type,
			nodes_size,
			ast->str_value,
			ast->int_value,
			ast->push);

	print(template, ast_str);
	free(ast_str);
}
