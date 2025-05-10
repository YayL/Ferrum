#include "codegen/checker.h"
#include "codegen/AST.h"
#include "common/hashmap.h"
#include "parser/operators.h"
#include "parser/types.h"

struct Ast * get_from_list(struct List * list, const char * name) {
    if (list == NULL)
        return 0;

    char * compare;
    struct Ast * ast;

    for (int i = 0; i < list->size; ++i) {
        ast = list_at(list, i);
        switch (ast->type) {
            case AST_VARIABLE:
                compare = ((a_variable *) ast->value)->name;
                break;
            default:
                println("search_list does not handle AST type: '{s}'", ast_type_to_str(ast->type));
                exit(1);
        }

        if (!strcmp(name, compare))
            return ast;
    }

    return NULL;
}

struct Ast * get_symbol(char * const name, struct Ast const * scope) {
    struct Ast * ast;

    while(scope->type != AST_ROOT) {
        switch(scope->type) {
            case AST_SCOPE:
            {
                ast = get_from_list(((a_scope *) scope->value)->variables, name);
                if (ast != NULL)
                    return ast;
                break;
            }
            case AST_MODULE:
            {
                ast = hashmap_get(((a_module *) scope->value)->symbols, name);

                if (ast != NULL)
                    return ast;
                break;
            }
            case AST_FUNCTION:
            {
                a_function * function = scope->value;
                

                ast = function->arguments;
                if (ast == NULL)
                    return NULL;
                
                ast = get_from_list(((a_expr *) ast->value)->children, name);
                if (ast != NULL)
                    return ast;
                
                ast = hashmap_get(function->template_types, name);
                if (ast != NULL)
                    return ast;
                break;
            }
            case AST_IMPL:
            {
                break;
            }
            default:
            {
                logger_log(format("get_symbol unrecognized type: {s}", ast_type_to_str(scope->type)), CHECKER, ERROR);
                exit(1);
            }
        }
        scope = scope->scope;
    }
     
    return hashmap_get(((a_root *) scope->value)->markers, name);

}

struct Ast * get_variable(struct Ast * variable) {
    return get_symbol(((a_variable *) variable->value)->name, variable->scope);
}

char * get_name(struct Ast * ast) {
    switch (ast->type) {
        case AST_FUNCTION:
            return ((a_function *) ast->value)->name;
        case AST_DECLARATION:
            return ((a_variable *) ((a_declaration *) ast->value)->variable->value)->name;
        default:
            print_ast("get_name invalid type: {s}", ast);
            exit(1);
    }
}

void add_marker(struct Ast * marker, const char * name) {
    a_root * root = get_scope(AST_ROOT, marker)->value;

    hashmap_set(root->markers, name, marker);
}

void add_member_function(struct Ast * marker, struct Ast * new_member_to_add, struct Ast * current_scope) {
    current_scope = get_scope(AST_ROOT, current_scope);

    a_root * root = current_scope->value;
    a_type * type = marker->value;

    const char * marker_name = get_base_type_str(type);
    ASSERT1(marker_name != NULL);

    struct List * members = hashmap_get(root->markers, marker_name);

    if (members == NULL) {
        members = init_list(sizeof(struct Ast *));
        ASSERT1(marker_name != NULL);
        hashmap_set(root->markers, marker_name, members);
    }

    list_push(members, new_member_to_add);
}

void checker_check_type(struct Ast * ast, struct Ast * type) {
    a_type * left = ast->value, * right = type->value;

    if (left->intrinsic != right->intrinsic) {
        logger_log(format("{2s::} Missmatched types; {s} is not the same as {s}", left->name, right->name), CHECKER, ERROR);
        exit(1);
    }
}

struct Ast * checker_check_expr_node(struct Ast * ast) {
    switch (ast->type) {
        case AST_OP:
            return checker_check_op(ast);
        case AST_VARIABLE:
            return checker_check_variable(ast);
        case AST_EXPR:
            return checker_check_expression(ast);
        case AST_LITERAL:
            return ((a_literal *)ast->value)->type;
        case AST_TYPE:
            return ast;
        default:
            logger_log(format("Unimplemented expr node type: {s}", ast_type_to_str(ast->type)), CHECKER, ERROR);
            exit(1);
    }
}

struct Ast * checker_check_op(struct Ast * ast) {
    struct Ast * left = NULL,
               * right = NULL;
    a_op * op = ast->value;

    if (op->op->key == CALL) {
        if (op->left->type != AST_VARIABLE)
            return NULL;

        a_variable * var = op->left->value;
        if (var->name[0] == '#') {
            print_ast("right: {s}\n", op->right);
            checker_check_expr_node(op->right);
            return NULL;
        }

        if (!is_declared_function(var->name, ast->scope)) {
            logger_log(format("Call to unknown function: '{s}'", var->name), CHECKER, WARN);
            return NULL;
        }

        right = checker_check_expr_node(op->right);
 
        struct List * func_args = ast_to_ast_type_list(right);
        left = get_declared_function(var->name, func_args, ast->scope);

        checker_check_function(left);

        if (left == NULL) {
            ASSERT1(right != NULL);
            ASSERT1(right->value != NULL);

            logger_log(format("Unknown function: {2s}", var->name, get_type_str(right)), CHECKER, ERROR);

            exit(1);
        }

        op->type = ((a_function *) left->value)->return_type;
        op->definition = left;

        return op->type;

    } else if (op->op->key == TERNARY && ((a_op *) op->right->value)->op->key != TERNARY_BODY) {
        logger_log("Detected an error with the ternary operator. This was most likely caused by operator precedence or nested ternary operators. To remedy this try applying parenthesis around the seperate ternary body entries", CHECKER, ERROR);
        exit(1);
    } else if (op->op->key == PARENTHESES) {
        op->type = checker_check_expr_node(op->right);
        return op->type;
    } else if (op->op->key == ASSIGNMENT) {
        struct Ast * get_address_ast = init_ast(AST_OP, ast->scope);
        a_op * get_address = get_address_ast->value;
        get_address->right = op->left;

        get_address->op = malloc(sizeof(struct Operator));
        *(get_address->op) = str_to_operator("&", UNARY_PRE, NULL);
        
        op->left = get_address_ast;

        left = checker_check_expr_node(op->left);
        /* exit(0); */
    } else if (op->left) {
        left = checker_check_expr_node(op->left);
    }

    right = checker_check_expr_node(op->right);

    struct Ast * self_type = NULL;

    struct Ast * func = get_function_for_operator(op->op, left, right, &self_type, ast->scope);
    struct a_function * f1 = func->value;

    checker_check_function(func);

    op->definition = func;
    if (get_base_type(((a_function *) func->value)->return_type->value)->intrinsic == ITemplate) {
        op->type = replace_self_in_type(((a_function *) func->value)->return_type, self_type);
    }
 
    return op->type;
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

struct Ast * checker_check_expression(struct Ast * ast) {
    struct Ast * node, * type_ast = init_ast(AST_TYPE, ast->scope);
    a_expr * expr = ast->value;
    a_type * type = type_ast->value;

    for (int i = 0; i < expr->children->size; ++i) {
        node = list_at(expr->children, i);
        if (node != NULL)
            checker_check_expr_node(node);
    }

    return expr->type = ast_to_type(ast);
}

struct Ast * checker_check_variable(struct Ast * ast) {
    struct Ast * var_ast = get_variable(ast);

    if (var_ast == NULL || !((a_variable *) var_ast->value)->is_declared) {
        a_variable * variable = ast->value;
        logger_log(format("Variable '{s}' used before having been declared", variable->name), CHECKER, ERROR);
        return NULL;
        exit(1);
    }

    if (ast->value != var_ast->value) {
        free(ast->value);
        ast->value = var_ast->value;
    }

    return ((a_variable *)ast->value)->type;
}

struct Ast * checker_check_struct(struct Ast * ast) {
    a_struct * _struct = ast->value;

    if (ast != get_symbol(_struct->name, ast->scope)) {
        logger_log(format("Multiple definitions for struct '{s}'", _struct->name), CHECKER, ERROR);
        exit(1);
    }

    _struct->type = ast_to_type(ast);

    return ast;
}

struct Ast * checker_check_impl(struct Ast * ast) {
    a_impl * impl = ast->value;
    struct Ast * node = get_symbol(impl->name, ast->scope),
               * temp1,
               * temp2;

    if (node == NULL) {
        logger_log(format("Invalid trait '{s}' for impl", impl->name), CHECKER, ERROR);
        exit(1);
    }

    a_trait * trait = node->value;

    if (trait->children->size != impl->members->size) {
        logger_log(format("Trait '{s}' is not fully implemented for {s}", impl->name, type_to_str(impl->type->value)), CHECKER, ERROR);
        exit(1);
    }
    
    size_t size = impl->members->size;
    char * name1, * name2;

    for (int i = 0; i < size; ++i) {
        char found = 0;
        temp1 = list_at(impl->members, i);
        name1 = get_name(temp1);
        for (int j = 0; j < size; ++j) {
            temp2 = list_at(trait->children, j);
 
            if (temp1->type == temp2->type && !strcmp(name1, get_name(temp2))) {
                found = 1;
                break;
            }
        }
        if (!found) {
            logger_log(format("Trait '{s}' is not validly implemented for {s}. Correct member definitions", impl->name, type_to_str(impl->type->value)), CHECKER, ERROR);
            exit(1);
        }
    }
    
    a_type * type = impl->type->value;
    struct List * list;
    if (type->intrinsic != ITuple) {
        list = init_list(sizeof(struct Ast *));
        list_push(list, impl->type);
    } else {
        list = ((Tuple_T *) type->ptr)->types;
    }

    if (ast->scope->type != AST_MODULE) {
        logger_log("impl is not at module scope?", CHECKER, ERROR);
        exit(1);
    }

    a_module * module = ast->scope->value;

    size = list->size;
    for (int i = 0; i < size; ++i) {
        temp1 = list_at(list, i); // current type/marker that is to be added to
        for (int j = 0; j < impl->members->size; ++j) {
            temp2 = list_at(impl->members, j);
            checker_check_function(temp2);
            add_member_function(temp1, list_at(impl->members, j), ast->scope);
        }
    }
    
    return ast;

}

struct Ast * checker_check_trait(struct Ast * ast) {
    a_trait * trait = ast->value;

    struct Ast * node = get_symbol(trait->name, ast->scope);

    if (ast != node) {
        logger_log(format("Multiple definitions for trait '{s}'", trait->name), CHECKER, ERROR);
        exit(1);
    }

    return ast;

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
                            logger_log(format("Unsupported type '{s}' as variable holder", ast_type_to_str(ast->scope->type)), CHECKER, ERROR);
                            exit(1);
                        }
                }
            } else {
                logger_log("Declarations require a direct assignment", CHECKER, ERROR);
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
                        logger_log(format("Unsupported type '{s}' as variable holder", ast_type_to_str(ast->scope->type)), CHECKER, ERROR);
                        exit(1);
                    }
            }
        }

        checker_check_expr_node(node);
    }
}

/* TODO:
 * Add type templating
 * A type template is defined with a requirement or without one. 
 * A template requirement specifies whether a type should have a generated version of this function
 * The compiler will not generate a new AST for a new type. Instead it will copy the function AST node only and then add a template list which specifies the template types. 
 * With the template list it will then check the function body to check if this function is able to be generated with these template parameters.
 * When it is the generators time it will look up the template type it encounters and replace it with the type specified in the template list at the function scope
 */

void checker_check_function(struct Ast * ast) {
    struct Ast * node;
    a_function * function = ast->value;
 
    /* if (template_types != NULL && template_types->total == 0) */
    /*     function->template_types = template_types; */
    /* else */
    /*     function->template_types = NULL; */

    a_expr * arguments = function->arguments->value;

    for (int i = 0; i < arguments->children->size; ++i) {
        node = list_at(arguments->children, i);
        ASSERT(node->type == AST_VARIABLE, "Function arguments currently only support variable definition");
        ((a_variable *) node->value)->is_declared = 1;
        checker_check_variable(node);
    }

    checker_check_scope(function->body);
    /* function->template_types = NULL; */
}

struct Ast * checker_check_module(struct Ast * ast) {
    a_module * module = ast->value;
    int size;

    size = module->variables->size;
    for (int i = 0; i < size; ++i) {
        checker_check_variable(list_at(module->variables, i));
    }

    size = module->structures->size;
    for (int i = 0; i < size; ++i) {
        checker_check_struct(list_at(module->structures, i));
    }

    size = module->traits->size;
    for (int i = 0; i < size; ++i) {
        checker_check_trait(list_at(module->traits, i));
    }

    size = module->impls->size;
    for (int i = 0; i < size; ++i) {
        checker_check_impl(list_at(module->impls, i));
    }

    return hashmap_get(module->symbols, "main");
}

void checker_check(struct Ast * ast) {
    a_root * root = ast->value;
    struct Ast * main = NULL, * temp;

    for (int i = root->modules->size; i--;) {
        temp = checker_check_module(list_at(root->modules, i));
        if (temp != NULL) {
            if (main != NULL) {
                logger_log("Uhoh multiple definitions of main", CHECKER, FATAL);
                exit(1);
            }
            main = temp;
        }
    }

    if (main == NULL) {
        logger_log("There is no main function", CHECKER, ERROR);
        exit(1);
    }

    checker_check_function(main);
}
