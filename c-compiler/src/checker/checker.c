#include "checker/checker.h"

#include "codegen/builtin.h"
#include "checker/functions.h"
#include "checker/context.h"
#include "checker/symbol.h"
#include "checker/typing.h"
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

ID checker_check_expr_node(ID node_id) {
    switch (node_id.type) {
        case ID_AST_OP:
            return checker_check_op(node_id);
        case ID_AST_VARIABLE:
            return checker_check_variable(node_id);
        case ID_AST_EXPR:
            return checker_check_expression(node_id);
        case ID_AST_LITERAL:
            return checker_check_literal(node_id);
        case ID_AST_SYMBOL:
            return checker_check_symbol(node_id);
        default:
            FATAL("Unimplemented expr node type: {s}", id_type_to_string(node_id.type));
    }
}

ID checker_check_literal(ID node_id) {
    ASSERT1(ID_IS(node_id, ID_AST_LITERAL));

    a_literal literal = LOOKUP(node_id, a_literal);
    ASSERT1(!ID_IS_INVALID(literal.type_id));
    ASSERT1(literal.value._ptr != NULL && literal.value.length > 0)

    return literal.type_id;
}

ID checker_check_symbol(ID node_id) {
    ASSERT1(ID_IS(node_id, ID_AST_SYMBOL));
    a_symbol * symbol = lookup(node_id);

    if (!ID_IS_INVALID(symbol->node_id)) {
        return symbol->node_id;
    }

    if (symbol->name_ids.size > 1) {
        symbol->node_id = qualify_symbol(symbol, ID_AST_DECLARATION);
    } else {
        symbol->node_id = context_lookup_declaration(symbol->name_id);
    }

    if (ID_IS_INVALID(symbol->node_id)) {
        FATAL("Unable to find symbol: {s}", interner_lookup_str(symbol->name_id)._ptr);
    }

    return symbol->node_id;
}

ID checker_check_op_member_access(a_operator * op) {
    ASSERT1(op->op.key == MEMBER_ACCESS);
    ASSERT1(ID_IS(op->right_id, ID_AST_SYMBOL));

    a_symbol member = LOOKUP(op->right_id, a_symbol);
    ASSERT1(member.name_ids.size == 1);

    checker_check_expr_node(op->left_id);
    print_ast_tree(op->info.node_id);

    ID type = INVALID_ID;

    if (ID_IS(op->left_id, ID_AST_SYMBOL)) {
        a_symbol symbol = LOOKUP(op->left_id, a_symbol);
        ASSERT1(ID_IS(symbol.node_id, ID_AST_VARIABLE));
        type = LOOKUP(symbol.node_id, a_variable).type_id;
    } else if (ID_IS(op->left_id, ID_AST_OP)) {
        a_operator child_op = LOOKUP(op->left_id, a_operator);
        println("child op: {b}", !ID_IS_INVALID(child_op.definition.function_id));
        print_ast_tree(child_op.definition.function_id);
        exit(0);
    } else {
        FATAL("Unable to take member access of type: {s}", id_type_to_string(op->left_id.type));
    }

    // print_ast_tree(op->info.node_id);
    println("Type: {s}", type_to_str(type));

    Symbol_T type_symbol = LOOKUP(type, Symbol_T);

    ID qualified_symbol = qualify_symbol(lookup(type_symbol.symbol_id), ID_SYMBOL_TYPE);
    ASSERT1(!ID_IS_INVALID(qualified_symbol));

    // print_ast_tree(qualified_symbol);

    a_structure structure = LOOKUP(qualified_symbol, a_structure);
    ASSERT1(structure.templates.size == type_symbol.templates.size);
    khash_t(map_id_to_id) templates = kh_init(map_id_to_id);

    for (size_t i = 0; i < structure.templates.size; ++i) {
        ID template_symbol_id = ARENA_GET(structure.templates, i, ID);
        a_symbol symbol = LOOKUP(template_symbol_id, a_symbol);
        ASSERT1(symbol.name_ids.size == 1);
        ID structure_type_id = ARENA_GET(type_symbol.templates, i, ID);

        int retcode;
		khint_t k = kh_put(map_id_to_id, &templates, symbol.name_id, &retcode);
		if (retcode == KH_PUT_ALREADY_PRESENT) {
			FATAL("Duplicate template type: '{s}'", interner_lookup_str(symbol.name_id)._ptr);
		}

        // println("{s} = {s}", interner_lookup_str(symbol.name_id)._ptr, type_to_str(structure_type_id));
		ASSERT(retcode == KH_PUT_SUCCESS, "Unknown error occured while populating template hashmap: {i}", retcode);
		kh_value(&templates, k) = structure_type_id;
    }

    ID qualified_member = qualify_declaration(structure.declarations, member.name_id);
    ASSERT1(ID_IS(qualified_member, ID_AST_SYMBOL));
    a_symbol member_symbol = LOOKUP(qualified_member, a_symbol);
    ASSERT1(ID_IS(member_symbol.node_id, ID_AST_VARIABLE));

    switch (member_symbol.node_id.type) {
        case ID_AST_VARIABLE: {
            a_variable variable = LOOKUP(member_symbol.node_id, a_variable);
            ASSERT1(!ID_IS_INVALID(variable.type_id));

            return op->type_id = resolve_type_templates_in_type(variable.type_id, &templates);
        } break;
        default:
            FATAL("Unimplemented: {s}", id_type_to_string(member_symbol.node_id.type));
    }
}

ID checker_check_op_call(a_operator * op) {
    ASSERT1(op->op.key == CALL);

    if (ID_IS(op->left_id, ID_AST_OP)) {
        a_operator * left_op = lookup(op->left_id);
        switch (left_op->op.key) {
            case MEMBER_ACCESS: {
                checker_check_expr_node(left_op->left_id);
                resolve_function_from_call(op->info.node_id, (Arena) {0});
                return op->type_id;
            } break;
            case TEMPLATE: {
                ASSERT1(ID_IS(left_op->type_id, ID_TUPLE_TYPE));
                Arena templates = LOOKUP(left_op->type_id, Tuple_T).types;
                checker_check_expr_node(left_op->right_id);
                resolve_function_from_call(op->info.node_id, templates);
                return op->type_id;
            } break;
            default:
                FATAL("Arbitrary address calls are not supported yet");
        }
    }

    checker_check_expr_node(op->left_id);
    if (!ID_IS(op->left_id, ID_AST_SYMBOL)) {
        ERROR("Arbitrary address calls are not supported yet");
        return INVALID_ID;
    }

    a_symbol symbol = LOOKUP(op->left_id, a_symbol);
    if (builtin_interner_id_is_inbounds(symbol.name_id)) {
        if (symbol.name_ids.size != 1) {
            ERROR("Invalid to use '::' operator when trying to call builtin function");
            exit(1);
        }

        return op->type_id = VOID_TYPE;
    }

    checker_check_expr_node(op->right_id);
    resolve_function_from_call(op->info.node_id, (Arena) {0});

    return op->type_id;
}

ID checker_check_op(ID node_id) {
    a_operator * op = lookup(node_id);

    switch (op->op.key) {
        case CALL:
            return checker_check_op_call(op);
        case TERNARY:
            ASSERT1(ID_IS(op->right_id, ID_AST_OP));
            if (LOOKUP(op->right_id, a_operator).op.key != TERNARY_BODY) {
                FATAL("Detected an error with the ternary operator. This was most likely caused by operator precedence or nested ternary operators. To remedy this try applying parenthesis around the seperate ternary body entries");
            }
            break;
        case PARENTHESES:
            return op->type_id = checker_check_expr_node(op->right_id);
        case MEMBER_ACCESS:
            return checker_check_op_member_access(op);
        default:
            if (!ID_IS_INVALID(op->left_id)) {
                checker_check_expr_node(op->left_id);
            }
    }

    checker_check_expr_node(op->right_id);
    resolve_function_from_operator(node_id);

    return op->type_id;
}

ID checker_check_expression(ID node_id) {
    a_expression * expr = lookup(node_id);
    if (expr->children.size == 0) {
        return VOID_TYPE;
    }

    if (expr->children.size == 1) {
        ID child_node_id = ARENA_GET(expr->children, 0, ID);
        ASSERT1(!ID_IS_INVALID(child_node_id));
        return checker_check_expr_node(child_node_id);
    }

    Tuple_T * tuple_type = type_allocate(ID_TUPLE_TYPE);

    for (int i = 0; i < expr->children.size; ++i) {
        ID child_node_id = ARENA_GET(expr->children, i, ID);
        ASSERT1(!ID_IS_INVALID(child_node_id));
        ARENA_APPEND(&tuple_type->types, checker_check_expr_node(child_node_id));
    }

    return tuple_type->info.type_id;
}

ID checker_check_variable(ID node_id) {
    a_variable variable = LOOKUP(node_id, a_variable);
    ASSERT1(!ID_IS_INVALID(variable.type_id));
    return variable.type_id;
}

void checker_check_if(ID node_id) {
    ID next_id = node_id;
    
    do {
        a_if_statement if_statement = LOOKUP(next_id, a_if_statement);

        if (!ID_IS_INVALID(if_statement.expression_id)) {
            checker_check_expression(if_statement.expression_id);
        } else if (!ID_IS_INVALID(if_statement.next_id)) {
            FATAL("Else is not at the end of an if chain?");
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


void checker_check_struct(ID node_id) {
    a_structure _struct = LOOKUP(node_id, a_structure);
    context_add_template_list(_struct.templates);

    for (size_t i = 0; i < _struct.declarations.size; ++i) {
        println("{i}) {s}", i + 1, ast_to_string(ARENA_GET(_struct.declarations, i, ID)));
    }

    for (size_t i = 0; i < _struct.declarations.size; ++i) {
        ID child_node_id = ARENA_GET(_struct.declarations, i, ID);

        switch (child_node_id.type) {
            case ID_AST_DECLARATION: {
                // a_symbol symbol = LOOKUP(child_node_id, a_symbol);
                println("Checking {s}", ast_to_string(child_node_id));
                print_ast_tree(child_node_id);
                checker_check_declaration(child_node_id);
            } break;
            case ID_AST_FUNCTION: {
                // a_function function = LOOKUP(child_node_id, a_function);
                println("Checking {s}", ast_to_string(child_node_id));
                checker_check_function(child_node_id);
            } break;
            default:
                ERROR("Invalid ID type: {s}", id_type_to_string(child_node_id.type));
                exit(1);
        }

        // println("child: {s}", ast_to_string(child_node_id));
    }

    context_remove_template_list(_struct.templates);
}

void checker_check_impl(ID node_id) {
    // a_implementation impl = LOOKUP(node_id, a_implementation);
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
    // a_trait trait = LOOKUP(node_id, a_trait);

    // struct AST * node = get_symbol(trait.name, ast->scope);
    //
    // if (ast != node) {
    //     FATAL("Multiple definitions for trait '{s}'", trait.name);
    // }
}

void checker_check_scope(ID node_id) {
    a_scope scope = LOOKUP(node_id, a_scope);
    context_add_declaration_list(scope.declarations);

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

        // print_ast_tree(child_node_id);
    }

    context_remove_declaration_list(scope.declarations);
}

void checker_check_declaration(ID node_id) {
    a_declaration declaration = LOOKUP(node_id, a_declaration);
    a_expression * expr = lookup(declaration.expression_id);

    for (size_t i = 0; i < expr->children.size; ++i) {
        ID child_node_id = ARENA_GET(expr->children, i, ID);
        ID symbol_id = INVALID_ID;

        switch (child_node_id.type) {
            case ID_AST_OP: {
                a_operator assignment_op = LOOKUP(child_node_id, a_operator);

                if (assignment_op.op.key != ASSIGNMENT) {
                    FATAL("Declarations require a direct assignment");
                }

                symbol_id = assignment_op.left_id;
            } break;
            case ID_AST_SYMBOL: {
                symbol_id = child_node_id;
            } break;
            default:
                FATAL("Invalid declaration expr child: {s}", id_type_to_string(child_node_id.type));
        }

        a_variable * variable = lookup(LOOKUP(symbol_id, a_symbol).node_id);
        Place_T * place_type = type_allocate(ID_PLACE_TYPE);
        place_type->basetype_id = variable->type_id;
        place_type->is_mut = 1; // This is just temporary and the actual mutability is set after checker_check_expression

        variable->type_id = place_type->info.type_id;
    }

    checker_check_expression(declaration.expression_id);

    for (int i = 0; i < expr->children.size; ++i) {
        ID child_node_id = ARENA_GET(expr->children, i, ID);

        if (ID_IS(child_node_id, ID_AST_OP)) {
            a_operator assignment_op = LOOKUP(child_node_id, a_operator);
            ASSERT1(ID_IS(assignment_op.left_id, ID_AST_SYMBOL));
            a_variable * variable = lookup(LOOKUP(assignment_op.left_id, a_symbol).node_id);

            Place_T * place_type = lookup(variable->type_id);
            place_type->is_mut = declaration.is_mut;
            child_node_id = variable->info.node_id;
        } else {
            ASSERT1(ID_IS(child_node_id, ID_AST_SYMBOL));
            a_symbol symbol = LOOKUP(child_node_id, a_symbol);
            ASSERT1(ID_IS(symbol.node_id, ID_AST_VARIABLE));
            a_variable variable = LOOKUP(symbol.node_id, a_variable);

            if (ID_IS_INVALID(variable.type_id)) {
                FATAL("Empty variable declaration must include a type");
            }

            child_node_id = symbol.node_id;
        }

        checker_check_variable(child_node_id);
    }
}

void checker_check_function(ID node_id) {
    a_function function = LOOKUP(node_id, a_function);
    context_add_template_list(function.templates);
    a_expression arguments = LOOKUP(function.arguments_id, a_expression);
    context_add_declaration_list(arguments.children);

    for (int i = 0; i < arguments.children.size; ++i) {
        ID child_node_id = ARENA_GET(arguments.children, i, ID);
        ASSERT(ID_IS(child_node_id, ID_AST_SYMBOL), "Function arguments currently only support declarations");
        a_symbol symbol = LOOKUP(child_node_id, a_symbol);
        checker_check_variable(symbol.node_id);
    }

    checker_check_scope(function.body_id);

    context_remove_declaration_list(arguments.children);
    context_remove_template_list(function.templates);
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
        case ID_AST_GROUP: break;
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
    ID module_id;
    kh_foreach_value(&root.modules, module_id, {
        checker_check_module(module_id);
    });

    // perform main function lookup on root.entry_point module
}
