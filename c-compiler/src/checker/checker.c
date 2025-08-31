#include "checker/checker.h"

#include "codegen/builtin.h"
#include "checker/functions.h"
#include "checker/context.h"
#include "tables/registry_manager.h"

ID get_interner_id(ID node_id) {
    switch (node_id.type) {
        case ID_AST_FUNCTION:
            return LOOKUP(node_id, a_function).name_id;
        case ID_AST_DECLARATION:
            return LOOKUP(node_id, a_declaration).name_id;
        default:
            println("get_name invalid type: {s}", ast_to_string(node_id));
            exit(1);
    }
}

void checker_check_expr_node(ID node_id) {
    switch (node_id.type) {
        case ID_AST_OP:
            checker_check_op(node_id); break;
        case ID_AST_VARIABLE:
            checker_check_variable(node_id); break;
        case ID_AST_EXPR:
            checker_check_expression(node_id); break;
        case ID_AST_LITERAL:
            checker_check_literal(node_id); break;
        case ID_AST_SYMBOL:
            checker_check_symbol(node_id); break;
        default:
            FATAL("Unimplemented expr node type: {s}", id_type_to_string(node_id.type));
    }
}

void checker_check_literal(ID node_id) {
    ASSERT1(ID_IS(node_id, ID_AST_LITERAL));
    // ASSERT(0, "Unimplemented");
}

void checker_check_symbol(ID node_id) {
    ASSERT1(ID_IS(node_id, ID_AST_SYMBOL));
    a_symbol symbol = LOOKUP(node_id, a_symbol);

    // if (symbol.node == NULL) {
    //     println("Must lookup symbol: {s}", symbol_expand_path(ast));
    //
    //     for (size_t i = 0; i < 
    //
    //     exit(0);
    // }
}

void checker_check_op(ID node_id) {
    a_operator * op = lookup(node_id);

    if (op->op.key == CALL) {
        if (!ID_IS(op->left_id, ID_AST_SYMBOL)) {
            ERROR("Arbitrary address calls are not supported yet");
            return;
        }

        a_symbol symbol = LOOKUP(op->left_id, a_symbol);
        ID first_name_id = ARENA_GET(symbol.name_ids, 0, ID);
        if (builtin_interner_id_is_inbounds(first_name_id)) {
            if (symbol.name_ids.size != 1) {
                ERROR("Invalid to use '::' operator when trying to call builtin function");
                exit(1);
            }

            op->type_id = VOID_TYPE;
            return;
        }

        // if (!is_declared_function(var.name, ast->scope)) {
        //     ERROR("Call to unknown function: '{s}'", var.name);
        //     return UNKNOWN_TYPE;
        // }

        checker_check_expr_node(op->right_id);
 
        resolve_function_from_call(node_id);

        println("Resolved function call");

    } else if (op->op.key == TERNARY) {
        ASSERT1(ID_IS(op->right_id, ID_AST_OP));
        if (LOOKUP(op->right_id, a_operator).op.key != TERNARY_BODY) {
            FATAL("Detected an error with the ternary operator. This was most likely caused by operator precedence or nested ternary operators. To remedy this try applying parenthesis around the seperate ternary body entries");
        }
    } else if (op->op.key == PARENTHESES) {
        checker_check_expr_node(op->right_id);
        return;
    } else if (op->op.key == ASSIGNMENT) {
        a_operator * addr_of_op = ast_allocate(ID_AST_OP, ast_get_scope_id(node_id));

        addr_of_op->right_id = op->left_id;
        addr_of_op->op = str_to_operator("&", UNARY_PRE, NULL);
        op->left_id = addr_of_op->info.node_id;

        checker_check_expr_node(op->left_id);
        /* exit(0); */
    } else if (!ID_IS_INVALID(op->left_id)) {
        checker_check_expr_node(op->left_id);
    }

    checker_check_expr_node(op->right_id);


    resolve_function_from_operator(node_id);
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

void checker_check_if(ID node_id) {
    ID next_id = node_id;
    a_if_statement if_statement;
    
    do {
        if_statement = LOOKUP(node_id, a_if_statement);

        if (!ID_IS_INVALID(if_statement.expression_id)) {
            checker_check_expression(if_statement.expression_id);
        } else if (!ID_IS_INVALID(if_statement.next_id)) {
            ERROR("Else is only allowed at the end of an if chain");
            exit(1);
        }

        checker_check_scope(if_statement.body_id);

        if (ID_IS_INVALID(if_statement.next_id)) {
            break;
        }

        next_id = if_statement.next_id;
    } while (!ID_IS_INVALID(next_id));
}

void checker_check_while(ID node_id) {
    a_while_statement while_statement = LOOKUP(node_id, a_while_statement);

    checker_check_expression(while_statement.expression_id);
    checker_check_scope(while_statement.body_id);
}

void checker_check_for(ID node_id) {
    a_for_statement for_statement = LOOKUP(node_id, a_for_statement);

    checker_check_expression(for_statement.expression_id);
    checker_check_scope(for_statement.body_id);
}

void checker_check_return(ID node_id) {
    a_return_statement return_statement = LOOKUP(node_id, a_return_statement);

    checker_check_expression(return_statement.expression_id);
}

void checker_check_expression(ID node_id) {
    a_expression * expr = lookup(node_id);

    ASSERT1(expr->children.size != 0);
    for (int i = 0; i < expr->children.size; ++i) {
        ID child_node_id = ARENA_GET(expr->children, i, ID);
        ASSERT1(!ID_IS_INVALID(child_node_id));
        checker_check_expr_node(child_node_id);
    }

    // expr->type = ast_to_type(node_id);
}

void checker_check_variable(ID node_id) {
    // struct AST * var_ast = get_variable(ast);
    //
    // if (var_ast == NULL || !var_ast->value.variable.is_declared) {
    //     a_variable variable = ast->value.variable;
    //     // ERROR("Variable '{s}' used before having been declared", variable.name);
    //     return UNKNOWN_TYPE;
    // }

    // ast->value = var_ast->value;
}

void checker_check_struct(ID node_id) {
    a_structure _struct = LOOKUP(node_id, a_structure);

    // if (ast != get_symbol(_struct->name, ast->scope)) {
    //     FATAL("Multiple definitions for struct '{s}'", _struct->name);
    // }
}

void checker_check_impl(ID node_id) {
    a_implementation impl = LOOKUP(node_id, a_implementation);
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
    // if (ast->scope->type != ID_AST_MODULE) {
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

void checker_check_trait(ID node_id) {
    a_trait trait = LOOKUP(node_id, a_trait);

    // struct AST * node = get_symbol(trait.name, ast->scope);
    //
    // if (ast != node) {
    //     FATAL("Multiple definitions for trait '{s}'", trait.name);
    // }
}

void checker_check_scope(ID node_id) {
    a_scope scope = LOOKUP(node_id, a_scope);
    context_enter_scope(scope);

    for (int i = 0; i < scope.nodes.size; ++i) {
        ID child_node_id = ARENA_GET(scope.nodes, i, ID);
        switch (child_node_id.type) {
            case ID_AST_EXPR:
                checker_check_expression(child_node_id);
                break;
            case ID_AST_DECLARATION:
                checker_check_declaration(child_node_id);
                break;
            case ID_AST_IF:
                checker_check_if(child_node_id);
                break;
            case ID_AST_WHILE:
                checker_check_while(child_node_id);
                break;
            case ID_AST_FOR:
                checker_check_for(child_node_id);
                break;
            case ID_AST_RETURN:
                checker_check_return(child_node_id);
                break;
            default:
                ERROR("Invalid scope node type: {s}\n", id_type_to_string(child_node_id.type));
                break;
        }
    }

    context_exit_scope(scope);
}

void checker_check_declaration(ID node_id) {
    a_declaration declaration = LOOKUP(node_id, a_declaration);
    a_expression expr = LOOKUP(declaration.expression_id, a_expression);

    for (int i = 0; i < expr.children.size; ++i) {
        ID child_node_id = ARENA_GET(expr.children, i, ID);

        if (ID_IS(child_node_id, ID_AST_OP)) {
            a_operator assignment_op = LOOKUP(child_node_id, a_operator);

            if (assignment_op.op.key != ASSIGNMENT) {
                FATAL("Declarations require a direct assignment");
            }

            if (!ID_IS(assignment_op.left_id, ID_AST_VARIABLE)) {
                FATAL("On declaration the LHS must always be a variable");
            }

            checker_check_expr_node(assignment_op.right_id);
            child_node_id = assignment_op.left_id;
        }

        if (!ID_IS(child_node_id, ID_AST_VARIABLE)) {
            FATAL("Must be a variable in declaration");
        }

        a_variable * variable = lookup(child_node_id);
        variable->is_declared = 1;

        switch (variable->info.scope_id.type) {
            case ID_AST_SCOPE:
                {
                    // list_push(ast->scope->value.scope.variables, node);
                    break;
                }
            case ID_AST_MODULE:
                {
                    a_module * module = lookup(variable->info.scope_id);
                    ARENA_APPEND(&module->members, node_id);
                    break;
                }
            default:
                {
                    FATAL("Unsupported type '{s}' as variable holder", id_type_to_string(variable->info.scope_id.type));
                }
        }

        checker_check_expr_node(child_node_id);
    }
}

void checker_check_function(ID node_id) {
    a_function function = LOOKUP(node_id, a_function);
    context_enter_function(function);

    a_expression arguments = LOOKUP(function.arguments_id, a_expression);

    for (int i = 0; i < arguments.children.size; ++i) {
        ID child_node_id = ARENA_GET(arguments.children, i, ID);
        ASSERT(ID_IS(child_node_id, ID_AST_SYMBOL), "Function arguments currently only support declarations");
        a_symbol symbol = LOOKUP(child_node_id, a_symbol);
        a_variable * variable = lookup(symbol.node_id);

        variable->is_declared = 1;
        checker_check_variable(symbol.node_id);
    }

    checker_check_scope(function.body_id);

    context_exit_function(function);
}

void checker_check_import(ID node_id) {

}

void checker_check_definitions(ID node_id) {
    ASSERT1(!ID_IS_INVALID(node_id));

    switch (node_id.type) {
        case ID_AST_FUNCTION:
            checker_check_function(node_id); break;
        case ID_AST_DECLARATION:
            checker_check_declaration(node_id); break;
        case ID_AST_STRUCT:
            checker_check_struct(node_id); break;
        case ID_AST_TRAIT:
            checker_check_trait(node_id); break;
        case ID_AST_IMPL:
            checker_check_impl(node_id); break;
        case ID_AST_IMPORT:
            checker_check_import(node_id); break;
        default:
            FATAL("Invalid AST type: {s}", id_type_to_string(node_id.type));
    }
}

void checker_check_module(ID node_id) {
    ASSERT1(ID_IS(node_id, ID_AST_MODULE));
    a_module module = LOOKUP(node_id, a_module);
    context_enter_module(module);

    for (int i = 0; i < module.members.size; ++i) {
        checker_check_definitions(ARENA_GET(module.members, i, ID));
    }

    context_exit_module(module);
}

void checker_check(a_root root) {
    context_init();

    ID module_id;
    kh_foreach_value(&root.modules, module_id, {
        checker_check_module(module_id);
    });

    // perform main function lookup on root.entry_point module
}
