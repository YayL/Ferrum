#include "codegen/checker.h"

struct Ast * get_variable(struct Ast * variable) {
    
    struct Ast * scope = variable->scope,
               * ast;
    struct List * list;

    while(scope->type != AST_ROOT) {
        switch(scope->type) {
            case AST_SCOPE:
            {
                list = ((a_scope *) scope->value)->variables;
                break;
            }
            case AST_MODULE:
            {
                list = ((a_module *) scope->value)->variables;
                break;
            }
            case AST_FUNCTION:
            {
                list = ((a_function *) scope->value)->arguments;
                break;
            }
            default:
            {
                logger_log(format("get_variable unrecognized type: {s}", ast_type_to_str(scope->type)), PARSER, ERROR);
                exit(1);
            }
        }

        for (int i = 0; i < list->size; ++i) {
            ast = list_at(list, i);
            if (!strcmp(((a_variable *) ast->value)->name, ((a_variable *) variable->value)->name))
                return ast;
        }

        scope = scope->scope;
    }
    
    return NULL;
}

void checker_check_expr_node(struct Ast * ast) {
    switch (ast->type) {
        case AST_OP:
            checker_check_op(ast);
            break;
        case AST_VARIABLE:
            checker_check_variable(ast);
            break;
        default:
            break;
    }
}

void checker_check_op(struct Ast * ast) {
    struct Ast * node;
    a_op * op = ast->value;
    
    if (op->left) {
        checker_check_expr_node(op->left);
    }

    checker_check_expr_node(op->right);


}

void checker_check_if(struct Ast * ast) {

}

void checker_check_while(struct Ast * ast) {

}

void checker_check_expression(struct Ast * ast) {
    struct Ast * node;
    a_expr * expr = ast->value;

    for (int i = 0; i < expr->children->size; ++i) {
        node = list_at(expr->children, i);
        checker_check_expr_node(node);
    }

}

void checker_check_variable(struct Ast * ast) {
    struct Ast * var_ast = get_variable(ast);

    if (var_ast == NULL || !((a_variable *) var_ast->value)->is_declared) {
        a_variable * variable = ast->value;
        logger_log(format("Variable '{s}' used before having been declared", variable->name), PARSER, ERROR);
        exit(1);
    }

    if (ast->value != var_ast->value) {
        free(ast->value);
        ast->value = var_ast->value;
    }
}

void checker_check_scope(struct Ast * ast) {
    a_scope * scope = ast->value;
    struct Ast * node;

    for (int i = 0; i < scope->nodes->size; ++i) {
        node = list_at(scope->nodes, i);
        switch (node->type) {
            case AST_EXPR:
                checker_check_expression(node);
                break;
            case AST_DECLARATION:
                checker_check_declaration(node);
                break;
            case AST_IF:
                checker_check_if(node);
                break;
            case AST_WHILE:
                checker_check_while(node);
                break;
        }
    }

}

void checker_check_declaration(struct Ast * ast) {
    struct Ast * node;
    a_declaration * declaration = ast->value;
    a_expr * expr = declaration->expression->value;

    for (int i = 0; i < expr->children->size; ++i) {
        node = list_at(expr->children, i);
        print_ast("{s}\n", node);
        if (node->type == AST_OP) {
            a_op * op = node->value;
            if (op->op->key == ASSIGNMENT) {
                if (op->left->type != AST_VARIABLE) {
                    logger_log("On declaration the LHS must always be a variable", PARSER, ERROR);
                    exit(1);
                }
                checker_check_expr_node(op->right);
                node = get_variable(op->left);
                print_ast("{s}\n", node);
                ((a_variable *) node->value)->is_declared = 1;
            } else {
                logger_log("Upon declaration the assignment operator is the only operator allowed", PARSER, ERROR);
                exit(1);
            }
        } else {
            checker_check_expr_node(node);
        }
    }
}

void checker_check_function(struct Ast * ast) {
    struct Ast * node;
    a_function * function = ast->value;

    for (int i = 0; i < function->arguments->size; ++i) {
        node = list_at(function->arguments, i);
        ((a_variable *) node->value)->is_declared = 1;
        checker_check_variable(node);
    }

    checker_check_scope(function->body);
}

void checker_check_module(struct Ast * ast) {
    a_module * module = ast->value;
    int size = module->variables->size;

    for (int i = 0; i < size; ++i) {
        checker_check_variable(list_at(module->variables, i));
    }
    
    size = module->functions->size;
    for (int i = 0; i < size; ++i) {
        checker_check_function(list_at(module->functions, i));
    }
}

void checker_check(struct Ast * ast) {
    a_root * root = ast->value;

    for (int i = 0; i < root->modules->size; ++i) {
        checker_check_module(list_at(root->modules, i));
    }
}
