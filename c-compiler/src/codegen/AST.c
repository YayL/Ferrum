#include "codegen/AST.h"
#include "common/hashmap.h"
#include "common/string.h"

#define PADDING_DIRECT_CHILD  ""
#define PADDING_LIST_CHILDREN "│"
#define AST_TREE_PADDING(comp) (comp ? PADDING_LIST_CHILDREN : PADDING_DIRECT_CHILD)
#define AST_TREE_PRINT_CHILDREN(list, pstring) {for (int i = 0; i < list->size; ++i){ _print_ast_tree(list_at(list, i), pstring, i == (list->size - 1));}}

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
            root->markers = init_hashmap(8);

            return root;
        }
        case AST_MODULE:
        {
            a_module * module = malloc(sizeof(a_module));
            module->symbols = init_hashmap(8);
            module->functions = init_list(sizeof(struct Ast *));
            module->functions_map = init_hashmap(6);
            module->variables = init_list(sizeof(struct Ast *));
            module->structures = init_list(sizeof(struct Ast *));
            module->traits = init_list(sizeof(struct Ast *));
            module->impls = init_list(sizeof(struct Ast *));

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
        case AST_STRUCT:
        {
            a_struct * _struct = malloc(sizeof(a_struct));
            _struct->name = NULL;
            _struct->generics = init_list(sizeof(String *));
            _struct->functions = init_list(sizeof(struct Ast *));
            _struct->variables = init_list(sizeof(struct Ast *));

            return _struct;
        }
        case AST_TRAIT:
        {
            a_trait * trait = calloc(1, sizeof(a_trait));
            trait->children = init_list(sizeof(struct Ast *));
            trait->impls = init_list(sizeof(struct Ast *));

            return trait;
        }
        case AST_IMPL:
        {
            a_impl * impl = malloc(sizeof(a_impl));

            return impl;
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
        case AST_CALL: return "Call";
		case AST_OP: return "Operator";
		case AST_EXPR: return "Expression";
		case AST_BREAK: return "Break";
        case AST_CONTINUE: return "Continue";
        case AST_DO: return "Do";
        case AST_MATCH: return "Match";
        case AST_STRUCT: return "Struct";
        case AST_ENUM: return "Enum";
        case AST_IMPL: return "Impl";
        case AST_TRAIT: return "Trait";
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

void _print_ast_tree(struct Ast * ast, String * pad, char is_last) { 
    // pad->size == 2 means that it has only appended two
    if (pad->size != 0) {
        print("{2s}", pad->_ptr, is_last ? "└─" : "├─");
    }
    
    if (ast == NULL) {
        println("(NULL)");
        return;
    }

    print_ast("{s}\n", ast);

    // create a new pointer so that this will not influence the next children
    pad = string_copy(pad);
    if (is_last) {
        string_append(pad, "   ");
    } else {
        string_append(pad, "│  ");
    }

    switch (ast->type) {
        case AST_ROOT:
        {
            a_root * root = ast->value;

            String * next_pad = string_copy(pad);
            AST_TREE_PRINT_CHILDREN(root->modules, next_pad);
            free_string(&next_pad);
            
            break;
        }
        case AST_MODULE:
        {
#ifdef SHALLOW_PRINT
            break;
#endif
            a_module * module = ast->value;

            String * next_pad = string_copy(pad);
            AST_TREE_PRINT_CHILDREN(module->variables, next_pad);
            free_string(&next_pad);

            next_pad = string_copy(pad);
            AST_TREE_PRINT_CHILDREN(module->impls, next_pad);
            free_string(&next_pad);

            next_pad = string_copy(pad);
            AST_TREE_PRINT_CHILDREN(module->functions, next_pad);
            free_string(&next_pad);

            break;
        }
        case AST_FUNCTION:
        {
            a_function * func = ast->value;
            
            String * next_pad = string_copy(pad);

            _print_ast_tree(func->arguments, next_pad, 0);
            _print_ast_tree(func->body, next_pad, 1);
            free_string(&next_pad);

            break;
        }
        case AST_SCOPE:
        {
            a_scope * scope = ast->value;     
            String * next_pad = string_copy(pad);
            struct List * list = list_combine(scope->variables, scope->nodes);

            AST_TREE_PRINT_CHILDREN(list, next_pad);
            free_string(&next_pad);
        } break;
#ifdef IMPL_PRINT
        case AST_IMPL:
        {
            a_impl * impl = ast->value;
            String * next_pad = string_copy(pad);
            
            AST_TREE_PRINT_CHILDREN(impl->members, next_pad);
            free_string(&next_pad);

        } break;
#endif
        case AST_DECLARATION:
        {
            a_declaration * decl = ast->value;
            
            String * next_pad = string_copy(pad);
            _print_ast_tree(decl->expression, next_pad, 1);
            free_string(&next_pad);

            break;
        }
        case AST_EXPR:
        {
            a_expr * expr = ast->value;

            String * next_pad = string_copy(pad);
            AST_TREE_PRINT_CHILDREN(expr->children, next_pad);
            free_string(&next_pad);
            
            break;
        }
        case AST_OP:
        {
            a_op * op = ast->value;

            String * next_pad = string_copy(pad);
            if (op->op->mode == BINARY) {
                _print_ast_tree(op->left, next_pad, 0);
                _print_ast_tree(op->right, next_pad, 1);
            } else {
                string_append(next_pad, PADDING_DIRECT_CHILD);
                _print_ast_tree(op->right, next_pad, 1);
            }
            free_string(&next_pad);

            break;
        }
        case AST_RETURN:
        {
            a_return * ret = ast->value;

            if (ret->expression) {
                String * next_pad = string_copy(pad);
                _print_ast_tree(ret->expression, next_pad, 1);
                free_string(&next_pad);
            }

            break;
        }
        case AST_FOR:
        {
            a_for_statement * for_statement = ast->value;

            String * next_pad = string_copy(pad);
            _print_ast_tree(for_statement->expression, next_pad, 0);
            _print_ast_tree(for_statement->body, next_pad, 1);
            free_string(&next_pad);

            break;
        }
        case AST_WHILE:
        {
            a_while_statement * while_statement = ast->value;

            String * next_pad = string_copy(pad);
            _print_ast_tree(while_statement->expression, next_pad, 0);
            _print_ast_tree(while_statement->body, next_pad, 1);
            free_string(&next_pad);

            break;
        }
        case AST_IF:
        {
            a_if_statement * if_statement = ast->value;

            String * next_pad = string_copy(pad);
            
            while (1) {
                if (if_statement->expression) {
                    _print_ast_tree(if_statement->expression, next_pad, 0);
                    _print_ast_tree(if_statement->body, next_pad, if_statement->next == NULL);
                } else {
                    _print_ast_tree(if_statement->body, next_pad, 1);
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
    _print_ast_tree(ast, string, 1);
    free_string(&string);
}

void print_ast(const char * template, struct Ast * ast) {
	const char * type_str = ast_type_to_str_ast(ast);
	const char * scope = ast_type_to_str_ast(ast->scope);
    
    char * ast_str = format(RED "{s}" RESET ": ", type_str);

    switch (ast->type) {
        case AST_MODULE:
        {
            a_module * module = ast->value;
            ast_str = format("{s} " GREY "<" BLUE "Symbols" RESET ": {i}, " BLUE "Path" RESET ": {s}" GREY ">" RESET, ast_str, module->symbols->total, module->path);
            break;
        }
        case AST_FUNCTION:
        {
            a_function * func = ast->value;
            ast_str = format("{s} " GREY "<" BLUE "Name" RESET ": {s}, " BLUE "Type" RESET ": {2s: -> }" GREY ">" RESET, ast_str, func->name, get_type_str(func->param_type), get_type_str(func->return_type));
            break;
        }
        case AST_SCOPE:
        {
            a_scope * scope = ast->value;
            ast_str = format("{s} " GREY "<" BLUE "Variables" RESET ": {i}, " BLUE "Nodes" RESET ": {i}" GREY ">" RESET, ast_str, scope->variables->size, scope->nodes->size);
        } break;
        case AST_IMPL:
        {
            a_impl * impl = ast->value;
            ast_str = format("{s} " GREY "<" BLUE "Name" RESET ": {s}, " BLUE "Types" RESET ": {s}" GREY ">" RESET, ast_str, impl->name, get_type_str(impl->type));
        } break;
        case AST_OP:
        {
            a_op * op = ast->value;
            ast_str = format("{s} " GREY "<" BLUE "Op" RESET ": '{s}', " BLUE "Mode" RESET ": {s}, " BLUE "Type" RESET ": {s}" GREY ">" RESET, ast_str, op->op ? op->op->str : "(NULL)", op->op->mode == BINARY ? "Binary" : "Unary", op->type != NULL ? type_to_str(op->type->value) : "(NULL)");
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
            ast_str = format("{s} " GREY "<" BLUE "Name" RESET ": {s}" GREY ">" RESET, ast_str, get_type_str(ast));
            break;
        }
        case AST_LITERAL:
        {
            a_literal * literal = ast->value;
            const char * fmt_string;

            switch (literal->literal_type) {
                case LITERAL_NUMBER:
                    fmt_string = "{s} " GREY "<" BLUE "Literal" RESET ": Number, " BLUE "Type" RESET ": {s}, " BLUE "Value" RESET ": {s}" GREY ">" RESET;
                    break;
                case LITERAL_STRING:
                    fmt_string = "{s} " GREY "<" BLUE "Literal" RESET ": String, " BLUE "Type" RESET ": {s}, " BLUE "Value" RESET ": \"{s}\"" GREY ">" RESET;
                    break;
            }

            ast_str = format(fmt_string, ast_str, type_to_str(literal->type->value), literal->value);
            break;
        }
        case AST_DECLARATION:
        {
            a_declaration * declaration = ast->value;
            ast_str = format("{s} " GREY "<" BLUE "Type" RESET ": {s}" GREY ">" RESET, ast_str, declaration->is_const ? "Constant" : "Variable");
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

struct Ast * ast_get_type_of(struct Ast * ast) {
    switch (ast->type) {
        case AST_OP:
        {
            a_op * op = ast->value;
            // ASSERT1(op->type != NULL);
            return op->type;
        }
        case AST_LITERAL:
        {
            return ((a_literal *) ast->value)->type;
        }
        case AST_VARIABLE:
        {
            return ((a_variable *) ast->value)->type;
        }
        default:
            logger_log(format("Unable to get a type from ast type '{s}'", ast_type_to_str(ast->type)), CHECKER, ERROR);
            exit(1);
    }
}

struct Ast * get_scope(enum AST_type type, struct Ast * scope) {
    while (scope->type != AST_ROOT && scope->type != type) {
        scope = scope->scope;
    }

    return scope;
}
