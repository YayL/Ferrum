#include "checker/checker.h"
#include "checker/context.h"
#include "checker/symbols.h"
#include "codegen/AST.h"
#include "codegen/builtin.h"
#include "codegen/functions.h"
#include "common/arena.h"
#include "common/hashmap.h"
#include "common/logger.h"
#include "common/macro.h"
#include "fmt.h"
#include "parser/operators.h"
#include "parser/types.h"
#include "tables/interner.h"

ID get_interner_id(struct AST * ast) {
    switch (ast->type) {
        case AST_FUNCTION:
            return ast->value.function.name_id;
        case AST_DECLARATION:
            return ast->value.declaration.name_id;
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

void checker_check_expr_node(struct AST * ast) {
    switch (ast->type) {
        case AST_OP:
            checker_check_op(ast); break;
        case AST_VARIABLE:
            checker_check_variable(ast); break;
        case AST_EXPR:
            checker_check_expression(ast); break;
        case AST_LITERAL:
            checker_check_literal(ast); break;
        case AST_SYMBOL:
            checker_check_symbol(ast); break;
        default:
            FATAL("Unimplemented expr node type: {s}", ast_type_to_str(ast->type));
    }
}   

void checker_check_literal(struct AST * ast) {
    ASSERT1(ast->type == AST_LITERAL);
    // ASSERT(0, "Unimplemented");
}

void checker_check_symbol(struct AST * ast) {
    ASSERT1(ast->type == AST_SYMBOL);
    a_symbol symbol = ast->value.symbol;

    // if (symbol.node == NULL) {
    //     println("Must lookup symbol: {s}", symbol_expand_path(ast));
    //
    //     for (size_t i = 0; i < 
    //
    //     exit(0);
    // }
}

void checker_check_op(struct AST * ast) {
    Type left = UNKNOWN_TYPE, right = UNKNOWN_TYPE;
    a_operator * op = &ast->value.operator;

    if (op->op->key == CALL) {
        if (op->left->type != AST_SYMBOL) {
            ERROR("Arbitrary address calls are not supported yet");
            return;
        }

        a_symbol symbol = op->left->value.symbol;
        unsigned int first_name_id = ARENA_GET(symbol.name_ids, 0, unsigned int);
        if (builtin_interner_id_is_inbounds(first_name_id)) {
            if (symbol.name_ids.size != 1) {
                ERROR("Invalid to use '::' operator when trying to call builtin function");
                exit(1);
            }

            op->type = &VOID_TYPE;
            return;
        }

        // if (!is_declared_function(var.name, ast->scope)) {
        //     ERROR("Call to unknown function: '{s}'", var.name);
        //     return UNKNOWN_TYPE;
        // }

        checker_check_expr_node(op->right);
 
        resolve_function_from_call(ast);

        println("Resolved function call");

    } else if (op->op->key == TERNARY && op->right->value.operator.op->key != TERNARY_BODY) {
        FATAL("Detected an error with the ternary operator. This was most likely caused by operator precedence or nested ternary operators. To remedy this try applying parenthesis around the seperate ternary body entries");
    } else if (op->op->key == PARENTHESES) {
        ALLOC(op->type);
        checker_check_expr_node(op->right);
        return;
    } else if (op->op->key == ASSIGNMENT) {
        struct AST * get_address_ast = init_ast(AST_OP, ast->scope);
        a_operator * get_address = &get_address_ast->value.operator;
        get_address->right = op->left;

        get_address->op = malloc(sizeof(struct Operator));
        *(get_address->op) = str_to_operator("&", UNARY_PRE, NULL);
        
        op->left = get_address_ast;

        checker_check_expr_node(op->left);
        /* exit(0); */
    } else if (op->left) {
        checker_check_expr_node(op->left);
    }

    checker_check_expr_node(op->right);

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
    a_return_statement return_statement = ast->value.return_statement;

    checker_check_expression(return_statement.expression);
}

void checker_check_expression(struct AST * ast) {
    a_expression * expr = &ast->value.expression;

    ASSERT1(expr->children.size != 0);
    for (int i = 0; i < expr->children.size; ++i) {
        struct AST * node = ARENA_GET(expr->children, i, struct AST *);
        ASSERT1(node != NULL);
        checker_check_expr_node(node);
    }

    ALLOC(expr->type);
    *expr->type = ast_to_type(ast);
}

void checker_check_variable(struct AST * ast) {
    // struct AST * var_ast = get_variable(ast);
    //
    // if (var_ast == NULL || !var_ast->value.variable.is_declared) {
    //     a_variable variable = ast->value.variable;
    //     // ERROR("Variable '{s}' used before having been declared", variable.name);
    //     return UNKNOWN_TYPE;
    // }

    // ast->value = var_ast->value;
}

struct AST * checker_check_struct(struct AST * ast) {
    a_structure * _struct = &ast->value.structure;

    // if (ast != get_symbol(_struct->name, ast->scope)) {
    //     FATAL("Multiple definitions for struct '{s}'", _struct->name);
    // }

    return ast;
}

struct AST * checker_check_impl(struct AST * ast) {
    a_implementation impl = ast->value.implementation;
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
    context_enter_scope(ast);
    a_scope scope = ast->value.scope;

    for (int i = 0; i < scope.nodes.size; ++i) {
        struct AST * node = ARENA_GET(scope.nodes, i, struct AST *);
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

    context_exit_scope(ast);
}

void checker_check_declaration(struct AST * ast) {
    a_declaration declaration = ast->value.declaration;
    a_expression expr = declaration.expression->value.expression;

    for (int i = 0; i < expr.children.size; ++i) {
        struct AST * node = ARENA_GET(expr.children, i, struct AST *);
        if (node->type == AST_OP) {
            a_operator * op = &node->value.operator;
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
                            ARENA_APPEND(&ast->scope->value.module.members, op->left);
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
                        ARENA_APPEND(&ast->scope->value.module.members, node);
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

void checker_check_function(struct AST * ast) {
    context_enter_function(ast);

    a_function function = ast->value.function;
    a_expression arguments = function.arguments->value.expression;

    for (int i = 0; i < arguments.children.size; ++i) {
        struct AST * node = ARENA_GET(arguments.children, i, struct AST *);
        ASSERT(node->type == AST_SYMBOL, "Function arguments currently only support declarations");
        node->value.variable.is_declared = 1;
        checker_check_variable(node);
    }

    checker_check_scope(function.body);

    context_exit_function(ast);
}

void checker_check_import(struct AST * ast) {

}

void checker_check_definitions(struct AST * ast) {
    ASSERT1(ast != NULL);

    switch (ast->type) {
        case AST_FUNCTION:
            checker_check_function(ast); break;
        case AST_DECLARATION:
            checker_check_declaration(ast); break;
        case AST_STRUCT:
            checker_check_struct(ast); break;
        case AST_TRAIT:
            checker_check_trait(ast); break;
        case AST_IMPL:
            checker_check_impl(ast); break;
        case AST_IMPORT:
            checker_check_import(ast); break;
        default:
            FATAL("Invalid AST type: {s}", ast_type_to_str(ast->type));
    }
}

void checker_check_module(struct AST * ast) {
    ASSERT1(ast != NULL);
    ASSERT1(ast->type == AST_MODULE);

    a_module * module = &ast->value.module;
    context_enter_module(ast);

    for (int i = 0; i < module->members.size; ++i) {
        checker_check_definitions(ARENA_GET(module->members, i, struct AST *));
    }

    context_exit_module(ast);
}

void checker_check(struct AST * ast) {
    ASSERT1(ast->type == AST_ROOT);
    a_root root = ast->value.root;

    context_init();

    struct AST * module;
    kh_foreach_value(&root.modules, module, {
        checker_check_module(module);
    });

    // perform main function lookup on root.entry_point module
}
