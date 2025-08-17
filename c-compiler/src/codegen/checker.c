#include "codegen/checker.h"
#include "codegen/AST.h"
#include "codegen/functions.h"
#include "common/arena.h"
#include "common/logger.h"
#include "common/macro.h"
#include "parser/operators.h"
#include "parser/types.h"
#include "tables/interner.h"

unsigned int get_interner_id(struct AST * ast) {
    switch (ast->type) {
        case AST_FUNCTION:
            return ast->value.function.interner_id;
        case AST_DECLARATION:
            return ast->value.declaration.variable->value.variable.interner_id;
        default:
            print_ast("get_name invalid type: {s}", ast);
            exit(1);
    }
}

void add_member_function(Type marker, struct AST * new_member_to_add, struct AST * current_scope) {
    current_scope = get_scope(AST_ROOT, current_scope);

    a_root root = current_scope->value.root;

    // DEBUG("add_member_function marker: {s}", type_to_str(marker));

    const char * marker_name = get_base_type_str(marker);
    ASSERT1(marker_name != NULL);

    // struct List * members = hashmap_get(root.markers, marker_name);

    // if (members == NULL) {
    //     members = init_list(sizeof(struct AST *));
    //     ASSERT1(marker_name != NULL);
    //     hashmap_set(root.markers, marker_name, members);
    // }
    //
    // list_push(members, new_member_to_add);
}

Type checker_check_expr_node(struct AST * ast) {
    switch (ast->type) {
        case AST_OP:
            return checker_check_op(ast);
        case AST_VARIABLE:
            return checker_check_variable(ast);
        case AST_EXPR:
            return checker_check_expression(ast);
        case AST_LITERAL:
            return *ast->value.literal.type;
        default:
            FATAL("Unimplemented expr node type: {s}", ast_type_to_str(ast->type));
    }
}

Type checker_check_op(struct AST * ast) {
    Type left = UNKNOWN_TYPE, right = UNKNOWN_TYPE;
    a_op * op = &ast->value.operator;

    if (op->op->key == CALL) {
        if (op->left->type != AST_VARIABLE) {
            ERROR("Arbitrary address calls are not supported yet");
            return UNKNOWN_TYPE;
        }

        a_variable var = op->left->value.variable;
        String func_call_name = interner_lookup_str(var.interner_id);
        if (func_call_name._ptr[0] == '#') {
            // return checker_check_expr_node(op->right);
            /* TODO: Give builtin function calls the correct return type */
            return UNKNOWN_TYPE;
        }

        // if (!is_declared_function(var.name, ast->scope)) {
        //     ERROR("Call to unknown function: '{s}'", var.name);
        //     return UNKNOWN_TYPE;
        // }

        right = checker_check_expr_node(op->right);
 
        resolve_function_from_call(ast);

    } else if (op->op->key == TERNARY && op->right->value.operator.op->key != TERNARY_BODY) {
        FATAL("Detected an error with the ternary operator. This was most likely caused by operator precedence or nested ternary operators. To remedy this try applying parenthesis around the seperate ternary body entries");
    } else if (op->op->key == PARENTHESES) {
        ALLOC(op->type);
        return *op->type = checker_check_expr_node(op->right);
    } else if (op->op->key == ASSIGNMENT) {
        struct AST * get_address_ast = init_ast(AST_OP, ast->scope);
        a_op * get_address = &get_address_ast->value.operator;
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

    Type self_type = {.intrinsic = IUnknown};

    resolve_function_from_operator(ast);
    // struct AST * func = get_function_for_operator(op->op, left, right, &self_type, ast->scope);
    // ASSERT(func != NULL, "get_function_for_operator returned NULL");
    // DEBUG("Found function: {s}", func->value.function.name);
    //
    // checker_check_function(func);
    //
    // op->definition = func;
    // ASSERT1(func->value.function.return_type != NULL);
    // if (get_base_type(*func->value.function.return_type).intrinsic == ITemplate) {
    //     if (op->type == NULL) {
    //         ALLOC(op->type);
    //     }
    //     *op->type = replace_self_in_type(*func->value.function.return_type, self_type);
    // }
    // 
    // ASSERT1(op->type != NULL);
    // return *op->type;
}

void checker_check_if(struct AST * ast) {
    a_if_statement if_statement = ast->value.if_statement;
    
    while (1) {
        if (if_statement.expression != NULL) {
            checker_check_expression(if_statement.expression);
        }

        checker_check_scope(if_statement.body);

        if (if_statement.next == NULL) {
            break;
        }

        if_statement = *if_statement.next;
    }
}

void checker_check_while(struct AST * ast) {
    a_while_statement while_statement = ast->value.while_statement;

    checker_check_expression(while_statement.expression);
    checker_check_scope(while_statement.body);
}

void checker_check_for(struct AST * ast) {
    a_for_statement for_statement = ast->value.for_statement;

    checker_check_expression(for_statement.expression);
    checker_check_scope(for_statement.body);
}

void checker_check_return(struct AST * ast) {
    a_return return_statement = ast->value.return_statement;

    checker_check_expression(return_statement.expression);
}

Type checker_check_expression(struct AST * ast) {
    struct AST * node;
    a_expr * expr = &ast->value.expression;

    for (int i = 0; i < expr->children->size; ++i) {
        node = list_at(expr->children, i);
        ASSERT1(node != NULL);
        checker_check_expr_node(node);
    }

    if (expr->type == NULL) {
        ALLOC(expr->type);
    }
    return *expr->type = ast_to_type(ast);
}

Type checker_check_variable(struct AST * ast) {
    // struct AST * var_ast = get_variable(ast);
    //
    // if (var_ast == NULL || !var_ast->value.variable.is_declared) {
    //     a_variable variable = ast->value.variable;
    //     // ERROR("Variable '{s}' used before having been declared", variable.name);
    //     return UNKNOWN_TYPE;
    // }

    // ast->value = var_ast->value;
    return *ast->value.variable.type;
}

struct AST * checker_check_struct(struct AST * ast) {
    a_struct * _struct = &ast->value.structure;

    // if (ast != get_symbol(_struct->name, ast->scope)) {
    //     FATAL("Multiple definitions for struct '{s}'", _struct->name);
    // }

    ALLOC(_struct->type);
    *_struct->type = ast_to_type(ast);
    return ast;
}

struct AST * checker_check_impl(struct AST * ast) {
    a_impl impl = ast->value.implementation;
    // struct AST * node = get_symbol(impl.name, ast->scope),
    //            * temp1,
    //            * temp2;
    //
    // if (node == NULL) {
    //     FATAL("Invalid trait '{s}' for impl", impl.name);
    // }
    //
    // a_trait trait = node->value.trait;
    //
    // if (trait.children->size != impl.members->size) {
    //     FATAL("Trait '{s}' is not fully implemented for {s}", impl.name, type_to_str(*impl.type));
    // }
    //
    // size_t size = impl.members->size;
    // char * name1, * name2;
    //
    // for (int i = 0; i < size; ++i) {
    //     char found = 0;
    //     temp1 = list_at(impl.members, i);
    //     name1 = get_name(temp1);
    //     for (int j = 0; j < size; ++j) {
    //         temp2 = list_at(trait.children, j);
    // 
    //         if (temp1->type == temp2->type && !strcmp(name1, get_name(temp2))) {
    //             found = 1;
    //             break;
    //         }
    //     }
    //     if (!found) {
    //         FATAL("Trait '{s}' is not validly implemented for {s}. Correct member definitions", impl.name, type_to_str(*impl.type));
    //     }
    // }
    //
    // struct arena arena;
    // if (impl.type->intrinsic != ITuple) {
    //     arena = arena_init(sizeof(Type));
    //     ARENA_APPEND(&arena, impl.type);
    // } else {
    //     arena = impl.type->value.tuple.types;
    // }
    //
    // if (ast->scope->type != AST_MODULE) {
    //     FATAL("impl is not at module scope?");
    // }
    //
    // a_module module = ast->scope->value.module;
    //
    // for (int i = 0; i < arena.size; ++i) {
    //     Type * type = arena_get(arena, i); // current type/marker that is to be added to
    //     ASSERT1(type != NULL);
    //     for (int j = 0; j < impl.members->size; ++j) {
    //         temp2 = list_at(impl.members, j);
    //         checker_check_function(temp2);
    //         add_member_function(*type, list_at(impl.members, j), ast->scope);
    //     }
    // }
    //
    // return ast;

}

struct AST * checker_check_trait(struct AST * ast) {
    a_trait trait = ast->value.trait;

    // struct AST * node = get_symbol(trait.name, ast->scope);
    //
    // if (ast != node) {
    //     FATAL("Multiple definitions for trait '{s}'", trait.name);
    // }

    return ast;

}

void checker_check_scope(struct AST * ast) {
    a_scope scope = ast->value.scope;
    struct AST * node;

    for (int i = 0; i < scope.nodes->size; ++i) {
        node = list_at(scope.nodes, i);
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
                ERROR("Invalid scope node type: {s}\n", ast_type_to_str(node->type));
                break;
        }
    }

}

void checker_check_declaration(struct AST * ast) {
    struct AST * node;
    a_declaration declaration = ast->value.declaration;
    a_expr expr = declaration.expression->value.expression;

    for (int i = 0; i < expr.children->size; ++i) {
        node = list_at(expr.children, i);
        if (node->type == AST_OP) {
            a_op * op = &node->value.operator;
            if (op->op->key == ASSIGNMENT) {
                if (op->left->type != AST_VARIABLE) {
                    FATAL("On declaration the LHS must always be a variable");
                }
                checker_check_expr_node(op->right);
                op->left->value.variable.is_declared = 1;

                switch (ast->scope->type) {
                    case AST_SCOPE:
                        {
                            // list_push(ast->scope->value.scope.variables, op->left);
                            break;
                        }
                    case AST_MODULE:
                        {
                            list_push(ast->scope->value.module.variables, op->left);
                            break;
                        }
                    default:
                        {
                            FATAL("Unsupported type '{s}' as variable holder", ast_type_to_str(ast->scope->type));
                        }
                }
            } else {
                FATAL("Declarations require a direct assignment");
            }
        } else if (node->type == AST_VARIABLE) {
            node->value.variable.is_declared = 1;

            switch (ast->scope->type) {
                case AST_SCOPE:
                    {
                        // list_push(ast->scope->value.scope.variables, node);
                        break;
                    }
                case AST_MODULE:
                    {
                        list_push(ast->scope->value.module.variables, node);
                        break;
                    }
                default:
                    {
                        FATAL("Unsupported type '{s}' as variable holder", ast_type_to_str(ast->scope->type));
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

void checker_check_function(struct AST * ast) {
    struct AST * node;
    a_function function = ast->value.function;
 
    /* if (template_types != NULL && template_types->total == 0) */
    /*     function->template_types = template_types; */
    /* else */
    /*     function->template_types = NULL; */

    a_expr arguments = function.arguments->value.expression;

    for (int i = 0; i < arguments.children->size; ++i) {
        node = list_at(arguments.children, i);
        ASSERT(node->type == AST_VARIABLE, "Function arguments currently only support variable definition");
        node->value.variable.is_declared = 1;
        checker_check_variable(node);
    }

    checker_check_scope(function.body);
    /* function->template_types = NULL; */
}

struct AST * checker_check_module(struct AST * ast) {
    a_module * module = &ast->value.module;
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

    // return hashmap_get(module->symbols, "main");
    return NULL;
}

void checker_check(struct AST * ast) {
    a_root root = ast->value.root;
    struct AST * main = NULL, * temp;

    for (int i = root.modules->size; i--;) {
        temp = checker_check_module(list_at(root.modules, i));
        if (temp != NULL) {
            if (main != NULL) {
                FATAL("Uhoh multiple definitions of main");
            }
            main = temp;
        }
    }

    if (main == NULL) {
        FATAL("There is no main function");
    }

    checker_check_function(main);
}
