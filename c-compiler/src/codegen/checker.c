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
                ast = ((a_function *) scope->value)->arguments;
                if (ast == NULL)
                    return NULL;
                list = ((a_expr *) ast->value)->children;
                break;
            }
            default:
            {
                logger_log(format("get_variable unrecognized type: {s}", ast_type_to_str(scope->type)), CHECKER, ERROR);
                exit(1);
            }
        }
        
        if (list != NULL) {
            for (int i = 0; i < list->size; ++i) {
                ast = list_at(list, i);
                if (!strcmp(((a_variable *) ast->value)->name, ((a_variable *) variable->value)->name))
                    return ast;
            }
        }

        scope = scope->scope;
    }
    
    return NULL;
}

char is_declared_function(char * name, struct Ast * scope) {
    while (scope->type != AST_ROOT) {
        if (scope->type == AST_MODULE) {
            a_module * module = scope->value;
            for (int i = 0; i < module->functions->size; ++i) {
                a_function * function = ((struct Ast *)list_at(module->functions, i))->value;
                if (!strcmp(function->name, name))
                    return 1;
            }
        }
        scope = scope->scope;
    }

    return 0;
}

void checker_check_type(struct Ast * ast, struct Ast * type) {
    a_type * left = ast->value, * right = type->value;

    if (left->intrinsic != right->intrinsic) {
        logger_log(format("{2s::} Missmatched types; {s} is not the same as {s}", left->name, right->name), CHECKER, ERROR);
        exit(1);
    }
}

a_type * checker_check_expr_node(struct Ast * ast) {
    switch (ast->type) {
        case AST_OP:
            return checker_check_op(ast);
            break;
        case AST_VARIABLE:
            return checker_check_variable(ast);
            break;
        case AST_EXPR:
            return checker_check_expression(ast);
            break;
        default:
            logger_log(format("Unimplemented expr node type: {s}", ast_type_to_str(ast->type)), CHECKER, ERROR);
            exit(1);
    }
}

a_type * checker_check_op(struct Ast * ast) {
    struct Ast * node;
    a_op * op = ast->value;
    a_type * left, * right;

    if (op->op->key == CALL && op->left->type == AST_VARIABLE) {
        a_variable * var = op->left->value;
        if (!is_declared_function(var->name, ast->scope)) {
            logger_log(format("Call to unknown function: '{s}'", var->name), CHECKER, WARN);
        }
    } else if (op->op->key == TERNARY && ((a_op *) op->right->value)->op->key != TERNARY_BODY) {
        logger_log("Detected an error with the ternary operator. This was most likely caused by operator precedence or nested ternary operators. To remedy this try applying parenthesis around the seperate ternary body entries", CHECKER, ERROR);
        exit(1);
    } else if (op->left) {
        left = checker_check_expr_node(op->left);
    }

    right = checker_check_expr_node(op->right);

    // TODO: search for an implementation of this operator with the types or otherwise error
}

void checker_check_if(struct Ast * ast) {
    a_if_statement * if_statement = ast->value;
    
    while (if_statement) {
        if (if_statement->expression)
            checker_check_expression(if_statement->expression);
        checker_check_scope(if_statement->body);

        if_statement = if_statement->next;
    }
}

void checker_check_while(struct Ast * ast) {
    a_while_statement * while_statement = ast->value;

    checker_check_expression(while_statement->expression);
    checker_check_scope(while_statement->body);
}

void checker_check_for(struct Ast * ast) {
    a_for_statement * for_statement = ast->value;

    checker_check_expression(for_statement->expression);
    checker_check_scope(for_statement->body);
}

void checker_check_return(struct Ast * ast) {
    a_return * return_statement = ast->value;

    checker_check_expression(return_statement->expression);
}

a_type * checker_check_expression(struct Ast * ast) {
    struct Ast * node;
    a_expr * expr = ast->value;

    for (int i = 0; i < expr->children->size; ++i) {
        node = list_at(expr->children, i);
        if (node != NULL)
            checker_check_expr_node(node);
    }

    // TODO: return a tuple type

}

a_type * checker_check_variable(struct Ast * ast) {
    struct Ast * var_ast = get_variable(ast);

    if (var_ast == NULL || !((a_variable *) var_ast->value)->is_declared) {
        a_variable * variable = ast->value;
        logger_log(format("Variable '{s}' used before having been declared", variable->name), CHECKER, ERROR);
        exit(1);
    }

    if (ast->value != var_ast->value) {
        free(ast->value);
        ast->value = var_ast->value;
    }

    return ((a_variable *)ast->value)->type->value;
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
            case AST_FOR:
                checker_check_for(node);
                break;
            case AST_RETURN:
                checker_check_return(node);
                break;
            default:
                logger_log(format("Invalid scope node type: {s}\n", ast_type_to_str(node->type)), CHECKER, ERROR);
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
        if (node->type == AST_OP) {
            a_op * op = node->value;
            if (op->op->key == ASSIGNMENT) {
                if (op->left->type != AST_VARIABLE) {
                    logger_log("On declaration the LHS must always be a variable", CHECKER, ERROR);
                    exit(1);
                }
                checker_check_expr_node(op->right);
                ((a_variable *) op->left->value)->is_declared = 1;

                switch (ast->scope->type) {
                    case AST_SCOPE:
                        {
                            list_push(((a_scope *) ast->scope->value)->variables, op->left);
                            break;
                        }
                    case AST_MODULE:
                        {
                            list_push(((a_module *) ast->scope->value)->variables, op->left);
                            break;
                        }
                    default:
                        {
                            logger_log(format("Unsupported type '{s}' as variable holder", ast_type_to_str(ast->scope->type)), PARSER, ERROR);
                            exit(1);
                        }
                }
            } else {
                logger_log("Upon declaration the assignment operator is the only operator allowed", CHECKER, ERROR);
                exit(1);
            }
        } else if (node->type == AST_VARIABLE) {
            ((a_variable *) node->value)->is_declared = 1;

            switch (ast->scope->type) {
                case AST_SCOPE:
                    {
                        list_push(((a_scope *) ast->scope->value)->variables, node);
                        break;
                    }
                case AST_MODULE:
                    {
                        list_push(((a_module *) ast->scope->value)->variables, node);
                        break;
                    }
                default:
                    {
                        logger_log(format("Unsupported type '{s}' as variable holder", ast_type_to_str(ast->scope->type)), PARSER, ERROR);
                        exit(1);
                    }
            }
        }

        checker_check_expr_node(node);
    }
}

void checker_check_function(struct Ast * ast) {
    struct Ast * node;
    a_function * function = ast->value;
    a_expr * arguments = function->arguments->value;

    for (int i = 0; i < arguments->children->size; ++i) {
        node = list_at(arguments->children, i);
        ASSERT(node->type == AST_VARIABLE, "Function arguments currently only support variable definition");
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
