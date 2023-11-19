#include "codegen/checker.h"
#include "codegen/AST.h"
#include "common/hashmap.h"
#include "parser/operators.h"
#include "parser/types.h"

struct Ast * get_symbol(char * const name, struct Ast const * scope) {    
    struct Ast * ast;
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
                ast = hashmap_get(((a_module *) scope->value)->symbols, name);
                if (ast != NULL)
                    return ast;
                list = NULL;
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
                if (!strcmp(((a_variable *) ast->value)->name, name))
                    return ast;
            }
        }

        scope = scope->scope;
    }
    
    return NULL;

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

struct Ast * get_declared_function(const char * name, struct List * list1, struct Ast * scope) {
    while (scope->type != AST_ROOT) {
        if (scope->type == AST_MODULE) {
            a_module * module = scope->value;
            for (int i = 0; i < module->functions->size; ++i) {
                struct Ast * node = list_at(module->functions, i);
                a_function * function = node->value;

                if (strcmp(function->name, name))
                    continue;

                struct List * list2 = ((Tuple_T *)((a_type *) function->param_type->value)->ptr)->types;

                if (list1->size != list2->size)
                    continue;

                char found = 1;
                for (int j = 0; j < list1->size; ++j) {
                    void * item1 = DEREF_AST(list_at(list1, j)),
                         * item2 = DEREF_AST(list_at(list2, j));
                    if (!is_equal_type(item1, item2, NULL)) {
                        print_ast("{s}\n", node);
                        found = 0;
                        break;
                    }
                }
                if (found)
                    return node;
            }
        }
        scope = scope->scope;
    }

    return NULL;
}

void add_member_function(struct Ast * marker, struct Ast * new_member_to_add, struct Ast * current_scope) {
    while (current_scope->type != AST_ROOT)
        current_scope = current_scope->scope;

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

struct Ast * get_member_function(struct Ast * marker, const char * member_name, 
                                 struct List * member_argument_types, struct Ast * current_scope) {
    if (marker == NULL) {
        logger_log(format("Unknown type for marker when calling '{s}'", member_name), CHECKER, ERROR);
        exit(1);
    }

    while (current_scope->type != AST_ROOT)
        current_scope = current_scope->scope;

    a_root * root = current_scope->value;
    a_type * type = marker->value;

    const char * marker_name = get_base_type_str(type);
    ASSERT1(marker_name != NULL);

    struct List * members = hashmap_get(root->markers, marker_name);
    if (members == NULL)
        return NULL;

    ASSERT(members->size != 0, "Member list is empty and stored in marker table??");

    const int size = members->size;
    struct Ast * node;
    for (int i = 0; i < size; ++i) {
        node = list_at(members, i);

        if (node->type != AST_FUNCTION) {
            logger_log("uhoh member must be function", CHECKER, ERROR);
            exit(1);
        }

        a_function * function = node->value;
        if (strcmp(function->name, member_name))
            continue;
        
        struct List * function_argument_types;
        
        Type * func_type = function->param_type->value;

        switch (func_type->intrinsic) {
            case ITuple:
                function_argument_types = ((Tuple_T *) func_type->ptr)->types; break;
            default:
                function_argument_types = init_list(sizeof(struct Ast *));
                list_push(function_argument_types, function->param_type);
        }

        if (function_argument_types->size != member_argument_types->size)
            continue;

        char found = 1;
        for (int j = 0; j < function_argument_types->size || !(found); ++j) {
            void * item1 = DEREF_AST(list_at(member_argument_types, j)), 
                 * item2 = DEREF_AST(list_at(function_argument_types, j));
            if (!is_equal_type(item1, item2, type) && !is_implicitly_equal(item1, item2, type)) {
                found = 0;
                break;
            }
        }

        if (found)
            return node;
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

struct Ast * get_function_for_operator(struct Operator * op, struct Ast * left, struct Ast * right, struct Ast ** self_type, struct Ast * scope) {
    struct List * temp_list = init_list(sizeof(struct Ast *));

    if (left != NULL) {
        list_push(temp_list, left);
    }
    list_push(temp_list, right);

    if (*self_type == NULL)
        *self_type = list_at(temp_list, 0);

    const char * name = get_operator_runtime_name(op->key);
    struct Ast * func = get_member_function(*self_type, name, temp_list, scope);

    if (func == NULL) {
        ASSERT1(right != NULL);
        ASSERT1(right->value != NULL);
    
        if (left != NULL)
            print_ast("left: {s}\n", left);
        print_ast("right: {s}\n", right);

        if (left != NULL)
            logger_log(format("Operator '{s}'({s}) is not defined for ({2s:, })", op->str, name, type_to_str(left->value), type_to_str(right->value)), CHECKER, ERROR);
        else 
            logger_log(format("Operator '{s}'({s}) is not defined for ({s})", op->str, name, type_to_str(right->value)), CHECKER, ERROR);

        exit(1);
    }

    return func;
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

struct Ast * checker_check_implicit(struct Ast * child, struct Ast * type, struct Ast ** self_type) {
    Type * t = type->value;

    switch (t->implicit) {
        case IE_EQUAL:
            return child;
        case IE_REFERENCE:
        {
            struct Ast * parent = init_ast(AST_OP, child->scope);
            a_op * op = parent->value;

            op->op = malloc(sizeof(struct Operator));
            *op->op = str_to_operator("&", UNARY_PRE, NULL);
            op->right = child;

            op->definition = get_function_for_operator(op->op, NULL, type, self_type, child->scope);
            op->type = replace_self_in_type(((a_function *) op->definition->value)->return_type, *self_type);

            t->implicit = IE_EQUAL;
            return parent;
        }
        case IE_DEREFERENCE:
        {
            struct Ast * parent = init_ast(AST_OP, child->scope);
            a_op * op = parent->value;

            op->op = malloc(sizeof(struct Operator));
            *op->op = str_to_operator("*", UNARY_PRE, NULL);
            op->right = child;

            op->definition = get_function_for_operator(op->op, NULL, type, self_type, child->scope);
            op->type = replace_self_in_type(((a_function *) op->definition->value)->return_type, *self_type);

            t->implicit = IE_EQUAL;
            return parent;
        }
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
            checker_check_expr_node(op->right);
            return NULL;
        }

        if (!is_declared_function(var->name, ast->scope)) {
            logger_log(format("Call to unknown function: '{s}'", var->name), CHECKER, WARN);
            return NULL;
        }

        right = checker_check_expr_node(op->right);
        ASSERT1(right->type == AST_TYPE);
        ASSERT1(((a_type *) right->value)->intrinsic == ITuple);
        left = get_declared_function(var->name, ((Tuple_T *)((a_type *) right->value)->ptr)->types, ast->scope);

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
    } else if (op->left) {
        left = checker_check_expr_node(op->left);
    }

    right = checker_check_expr_node(op->right);

    struct Ast * self_type = NULL;

    struct Ast * func = get_function_for_operator(op->op, left, right, &self_type, ast->scope);

    if (op->left) {
        op->left = checker_check_implicit(op->left, left, &self_type);
        op->right = checker_check_implicit(op->right, right, &self_type);
    } else {
        op->right = checker_check_implicit(op->right, right, &self_type);
    }

    op->definition = func;
    op->type = replace_self_in_type(((a_function *) func->value)->return_type, self_type);
 
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
                        logger_log(format("Unsupported type '{s}' as variable holder", ast_type_to_str(ast->scope->type)), CHECKER, ERROR);
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

    size = module->functions->size;
    for (int i = 0; i < size; ++i) {
        checker_check_function(list_at(module->functions, i));
    }
}

void checker_check(struct Ast * ast) {
    a_root * root = ast->value;

    for (int i = root->modules->size; i--;) {
        checker_check_module(list_at(root->modules, i));
    }
}
