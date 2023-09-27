#include "codegen/AST.h"

#define PADDING_DIRECT_CHILD "   "
#define PADDING_LIST_CHILDREN " │"
#define AST_TREE_PADDING(comp) (comp ? PADDING_LIST_CHILDREN : PADDING_DIRECT_CHILD)
#define AST_TREE_PRINT_CHILDREN(list, pstring) for (int i = 0; i < list->size; ++i){ _print_ast_tree(list_at(list, i), pstring, 1, i+1 == list->size);}

struct Ast * init_ast(enum AST_type type, struct Ast * scope) {
    struct Ast * ast = malloc(sizeof(struct Ast));
    ast->type = type;
    ast->scope = scope;
    ast->value = init_ast_of_type(type);

    return ast;
}

void * init_ast_of_type(enum AST_type type) {
    switch (type) {
        case AST_ROOT:
        {
            a_root * root = malloc(sizeof(a_root));
            root->modules = init_list(sizeof(a_module *));

            return root;
        }
        case AST_MODULE:
        {
            a_module * module = malloc(sizeof(a_module));
            module->functions = init_list(sizeof(struct Ast *));
            module->variables = init_list(sizeof(struct Ast *));

            return module;
        }
        case AST_FUNCTION:
        {
            a_function * function = calloc(1, sizeof(a_function));
            return function;
        }
        case AST_SCOPE:
        {
            a_scope * scope = malloc(sizeof(a_scope));
            scope->nodes = init_list(sizeof(struct Ast *));
            scope->variables = init_list(sizeof(struct Ast *));

            return scope;
        }
        case AST_DECLARATION:
        {
            a_declaration * decl = calloc(1, sizeof(a_declaration));
            return decl;
        }
        case AST_EXPR:
        {
            a_expr * expression = calloc(1, sizeof(a_expr));
            
            return expression;
        }
        case AST_OP:
        {
            a_op * op = calloc(1, sizeof(a_op));

            return op;
        }
        case AST_VARIABLE:
        {
            a_variable * variable = calloc(1, sizeof(a_variable));

            return variable;
        }
        case AST_TYPE:
        {
            a_type * type = calloc(1, sizeof(a_type));

            return type;
        }
        case AST_LITERAL:
        {
            a_literal * literal = calloc(1, sizeof(a_literal));

            return literal;
        }
        case AST_RETURN:
        {
            a_return * ret = calloc(1, sizeof(a_return));
            
            return ret;
        }
        case AST_FOR:
        {
            a_for_statement * for_statement = calloc(1, sizeof(a_for_statement));

            return for_statement;
        }
        case AST_WHILE:
        {
            a_while_statement * while_statement = calloc(1, sizeof(a_while_statement));

            return while_statement;
        }
        case AST_IF:
        {
            a_if_statement * if_statement = calloc(1, sizeof(a_if_statement));

            return if_statement;
        }
        default:
        {
            println("Unsupported type: {s}({i})", ast_type_to_str(type), type);
            exit(1);
        }
    }
}

const char * ast_type_to_str(enum AST_type type) {
	switch(type) {
        case AST_MODULE: return "Module";
		case AST_FUNCTION: return "Function";
        case AST_SCOPE: return "Scope";
        case AST_TYPE: return "Type";
        case AST_DECLARATION: return "Declaration";
		case AST_VARIABLE: return "Variable";
        case AST_LITERAL: return "Literal";
        case AST_ARRAY: return "Array";
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

const char * ast_type_to_str_ast(struct Ast * ast) {
    if (ast == NULL)
        return "(NULL)";
    
    return ast_type_to_str(ast->type);
}

void free_ast(struct Ast * ast) {
    
	free(ast);
}

void _print_ast_tree(struct Ast * ast, String * pad, char is_list, char is_last) {
    if (is_list)
        string_cut(pad, 1);
    
    print("{2s}", pad->_ptr, pad->size == 0 ? "" : ((is_last || !is_list) ? "└─" : "├─"));
    if (ast == NULL) {
        println("(NULL)");
        return;
    }

    print_ast("{s}\n", ast);

    if (is_list) {
        if (is_last)
            string_append(pad, " ");
        else
            string_append(pad, "│");
    }

    switch (ast->type) {
        case AST_ROOT:
        {
            a_root * root = ast->value;

            String * next_pad = string_copy(pad);
            string_append(next_pad, AST_TREE_PADDING(root->modules->size > 1));
            AST_TREE_PRINT_CHILDREN(root->modules, next_pad);
            free_string(&next_pad);
            
            break;
        }
        case AST_MODULE:
        {
            a_module * module = ast->value;

            String * next_pad = string_copy(pad);
            string_append(next_pad, AST_TREE_PADDING(module->variables->size > 1));
            AST_TREE_PRINT_CHILDREN(module->variables, next_pad);
            free_string(&next_pad);

            next_pad = string_copy(pad);
            string_append(next_pad, AST_TREE_PADDING(module->functions->size > 1));
            AST_TREE_PRINT_CHILDREN(module->functions, next_pad);
            free_string(&next_pad);

            break;
        }
        case AST_FUNCTION:
        {
            a_function * func = ast->value;
            
            String * next_pad = string_copy(pad);
            string_append(next_pad, AST_TREE_PADDING(1));

            _print_ast_tree(func->arguments, next_pad, 1, 1);
            _print_ast_tree(func->body, next_pad, 1, 1);
            free_string(&next_pad);

            break;
        }
        case AST_SCOPE:
        {
            a_scope * scope = ast->value; 
            
            String * next_pad = string_copy(pad);
            string_append(next_pad, AST_TREE_PADDING(scope->variables->size + scope->nodes->size > 1));
            AST_TREE_PRINT_CHILDREN(scope->variables, next_pad);            

            AST_TREE_PRINT_CHILDREN(scope->nodes, next_pad);
            free_string(&next_pad);

            break;
        }
        case AST_DECLARATION:
        {
            a_declaration * decl = ast->value;
            
            String * next_pad = string_copy(pad);
            string_append(next_pad, PADDING_DIRECT_CHILD);
            _print_ast_tree(decl->expression, next_pad, 0, 0);
            free_string(&next_pad);

            break;
        }
        case AST_EXPR:
        {
            a_expr * expr = ast->value;

            String * next_pad = string_copy(pad);
            string_append(next_pad, AST_TREE_PADDING(expr->children->size > 1));
            AST_TREE_PRINT_CHILDREN(expr->children, next_pad);
            free_string(&next_pad);
            
            break;
        }
        case AST_OP:
        {
            a_op * op = ast->value;

            String * next_pad = string_copy(pad);
            if (op->op->mode == BINARY) {
                string_append(next_pad, PADDING_LIST_CHILDREN);
                _print_ast_tree(op->left, next_pad, 1, 0);
                _print_ast_tree(op->right, next_pad, 1, 1);
            } else {
                string_append(next_pad, PADDING_DIRECT_CHILD);
                _print_ast_tree(op->right, next_pad, 0, 0);
            }
            free_string(&next_pad);

            break;
        }
        case AST_RETURN:
        {
            a_return * ret = ast->value;

            if (ret->expression) {
                String * next_pad = string_copy(pad);
                string_append(next_pad, PADDING_DIRECT_CHILD);
                _print_ast_tree(ret->expression, next_pad, 0, 0);
                free_string(&next_pad);
            }

            break;
        }
        case AST_FOR:
        {
            a_for_statement * for_statement = ast->value;

            String * next_pad = string_copy(pad);
            string_append(next_pad, PADDING_LIST_CHILDREN);
            _print_ast_tree(for_statement->expression, next_pad, 1, 0);
            _print_ast_tree(for_statement->body, next_pad, 1, 1);
            free_string(&next_pad);

            break;
        }
        case AST_WHILE:
        {
            a_while_statement * while_statement = ast->value;

            String * next_pad = string_copy(pad);
            string_append(next_pad, PADDING_LIST_CHILDREN);
            _print_ast_tree(while_statement->expression, next_pad, 1, 0);
            _print_ast_tree(while_statement->body, next_pad, 1, 1);
            free_string(&next_pad);

            break;
        }
        case AST_IF:
        {
            a_if_statement * if_statement = ast->value;

            String * next_pad = string_copy(pad);
            
            if (if_statement->expression || if_statement->next)
                string_append(next_pad, PADDING_LIST_CHILDREN);
            else
                string_append(next_pad, PADDING_DIRECT_CHILD);

            while (1) {
                if (if_statement->expression) {
                    _print_ast_tree(if_statement->expression, next_pad, 1, 0);
                    _print_ast_tree(if_statement->body, next_pad, 1, if_statement->next == NULL);
                } else {
                    _print_ast_tree(if_statement->body, next_pad, 0, 0);
                }
                if (if_statement->next == NULL)
                    break;
                if_statement = if_statement->next;
            }
            free_string(&next_pad);

            break;
        }
    } 
}

void print_ast_tree(struct Ast * ast) {
    String * string = init_string("");
    _print_ast_tree(ast, string, 0, 0);
    free_string(&string);
}

#define get_type_str(ast) (ast != NULL ? ((a_type *) ast->value)->name : "(NULL)")

void print_ast(const char * template, struct Ast * ast) {
	const char * type_str = ast_type_to_str_ast(ast);
	const char * scope = ast_type_to_str_ast(ast->scope);
    
    char * ast_str = format(RED "{s}" RESET ": ", type_str);

    switch (ast->type) {
        case AST_MODULE:
        {
            a_module * module = ast->value;
            ast_str = format("{s} " GREY "<" BLUE "Functions" RESET ": {i}, " BLUE "Variables" RESET ": {i}, " BLUE "Path" RESET ": '{s}'" GREY ">" RESET, ast_str, module->functions->size, module->variables->size, module->path);
            break;
        }
        case AST_FUNCTION:
        {
            a_function * func = ast->value;
            ast_str = format("{s} " GREY "<" BLUE "Name" RESET ": {s}, " BLUE "Type" RESET ": {s}" GREY ">" RESET, ast_str, func->name, get_type_str(func->type));
            break;
        }
        case AST_SCOPE:
        {
            a_scope * scope = ast->value;
            ast_str = format("{s} " GREY "<" BLUE "Variables" RESET ": {i}, " BLUE "Nodes" RESET ": {i}" GREY ">" RESET, ast_str, scope->variables->size, scope->nodes->size);
            break;
        }
        case AST_OP:
        {
            a_op * op = ast->value;
            ast_str = format("{s} " GREY "<" BLUE "Op" RESET ": '{s}', " BLUE "Mode" RESET ": {s}" GREY ">" RESET, ast_str, op->op ? op->op->str : "(NULL)", op->op->mode == BINARY ? "Binary" : "Unary");
            break;
        }
        case AST_VARIABLE:
        {
            a_variable * var = ast->value;
            ast_str = format("{s} " GREY "<" BLUE "Name" RESET ": {s}, " BLUE "Type" RESET ": {s}" GREY ">" RESET, ast_str, var->name, get_type_str(var->type));
            break;
        }
        case AST_TYPE:
        {
            a_type * type = ast->value;
            ast_str = format("{s} " GREY "<" BLUE "Name" RESET ": {s}" GREY ">" RESET, ast_str, type->name);
            break;
        }
        case AST_LITERAL:
        {
            a_literal * literal = ast->value;
            ast_str = format("{s} " GREY "<" BLUE "Type" RESET ": Number, " BLUE "Value" RESET ": {s}" GREY ">" RESET, ast_str, literal->value);
            break;
        }
        case AST_DECLARATION:
        {
            a_declaration * declaration = ast->value;
            ast_str = format("{s} " GREY "<" BLUE "Type" RESET ": {s}" GREY ">" RESET, ast_str, declaration->is_const ? "Variable" : "Constant");
            break;
        }
        default:
        {
            ast_str = format("{s} " GREY "<>" RESET, ast_str);
            break;
        }
    }

	print(template, ast_str);
	free(ast_str);
}
