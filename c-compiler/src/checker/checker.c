#include "checker/checker.h"

#include "parser/AST.h"
#include "checker/context.h"
#include "checker/symbol.h"
#include "tables/registry_manager.h"
#include "tables/member_functions.h"

char check_has_member(ID node_id, ID member_name_id) {
    ASSERT1(ID_IS(node_id, ID_AST_SYMBOL));
    a_symbol * symbol = lookup(node_id);

    if (ID_IS_INVALID(symbol->node_id)) {
        qualify_symbol(symbol, ID_SYMBOL_TYPE);
    }

    switch (symbol->node_id.type) {
        case ID_AST_STRUCT: {
            a_structure _struct = LOOKUP(symbol->node_id, a_structure);
            for (size_t i = 0; i < _struct.members.size; ++i) {
                if (id_is_equal(member_name_id, ast_get_interner_id(ARENA_GET(_struct.members, i, ID)))) {
                    return 1;
                }
            }
        } break;
        case ID_AST_IMPL: FATAL("Not implemented");
        default: return 0;
    }

    return 0;
}

ID checker_check_expr_node(ID node_id, const Arena templates) {
    ID ret_value;
    switch (node_id.type) {
        case ID_AST_OP:
            ret_value = checker_check_op(node_id, templates); break;
        case ID_AST_VARIABLE:
            ret_value = checker_check_variable(node_id, templates); break;
        case ID_AST_EXPR:
            ret_value = checker_check_expression(node_id, templates); break;
        case ID_AST_LITERAL:
            ret_value = checker_check_literal(node_id); break;
        case ID_AST_SYMBOL:
            ret_value = checker_check_symbol(node_id, templates); break;
        default:
            FATAL("Unimplemented expr node type: {s}", id_type_to_string(node_id.type));
    }

    // println("Expr node({s}): {s}", id_type_to_string(node_id.type), ast_to_string(ret_value));
    return ret_value;
}

ID checker_check_literal(ID node_id) {
    ASSERT1(ID_IS(node_id, ID_AST_LITERAL));

    a_literal literal = LOOKUP(node_id, a_literal);
    ASSERT1(!ID_IS_INVALID(literal.type_id));
    ASSERT1(literal.value._ptr != NULL && literal.value.length > 0)

    return literal.type_id;
}

ID checker_check_symbol(ID node_id, const Arena templates) {
    ASSERT1(ID_IS(node_id, ID_AST_SYMBOL));
    a_symbol * symbol = lookup(node_id);

    if (!ID_IS_INVALID(symbol->node_id)) {
        return replace_templates_in_type_with_template_variables(ast_get_type_of(symbol->node_id), templates, 0);
    }

    if (symbol->name_ids.size > 1) {
        symbol->node_id = qualify_symbol(symbol, ID_AST_DECLARATION);
    } else {
        symbol->node_id = context_lookup_declaration(symbol->name_id);
    }

    if (id_is_equal(symbol->info.scope_id, ast_get_scope_id(symbol->node_id))) {
        return replace_templates_in_type_with_template_variables(ast_get_type_of(symbol->node_id), templates, 0);
    }

    switch (symbol->node_id.type) {
        case ID_AST_FUNCTION: {
            Arena temp_templates = {0};
            generate_template_constraints(symbol->node_id, &temp_templates);
            return replace_templates_in_type_with_template_variables(ast_get_type_of(symbol->node_id), temp_templates, 0);
        } break;
        case ID_AST_VARIABLE: {
            Arena temp_templates = {0};
            generate_template_constraints(symbol->node_id, &temp_templates);
            ID type_id = replace_templates_in_type_with_template_variables(ast_get_type_of(symbol->node_id), temp_templates, 0);
            return type_id;
        }
        case ID_INVALID_TYPE: FATAL("Unable to find symbol: {s}", interner_lookup_str(symbol->name_id)._ptr);
        default: break;
    }

    return ast_get_type_of(symbol->node_id);
}

void checker_generate_member_function_call_dimension(a_operator * op, ID member_name_id, ID bounded_by_dimension) {
    Dimension_TC * dimension = tc_allocate(ID_TC_DIMENSION);
    dimension->candidates = member_function_index_lookup(member_name_id);
    op->dimension_id = dimension->dimension_id;

    Constraint_TC * constraint = tc_allocate(ID_TC_CONSTRAINT);
    constraint->from = bounded_by_dimension;
    constraint->to = dimension->dimension_id;
}

ID checker_check_op_member_access(a_operator * op, const Arena templates) {
    ID lhs_type_id = checker_check_expr_node(op->left_id, templates);

    Variable_TC * result_var = tc_allocate(ID_TC_VARIABLE);
    Shape_TC * shape = tc_allocate(ID_TC_SHAPE);

    ASSERT1(ID_IS(op->right_id, ID_AST_SYMBOL));
    shape->member_id = op->right_id;
    shape->requirement_id = result_var->variable_id;

    Constraint_TC * constraint = tc_allocate(ID_TC_CONSTRAINT);
    constraint->from = lhs_type_id;
    constraint->to = shape->shape_id;

    op->type_id = lhs_type_id;

    return shape->shape_id;
}

ID checker_check_op_call(a_operator * op, const Arena templates) {
    ASSERT1(!ID_IS_INVALID(op->left_id));
    // Signature to match against
    ID lhs_type_id = checker_check_expr_node(op->left_id, templates);

    // Get the args and turn it into a type
    ASSERT1(ID_IS(op->right_id, ID_AST_EXPR));
    a_expression args = LOOKUP(op->right_id, a_expression);

    Tuple_T * args_type = type_allocate(ID_TUPLE_TYPE);

    if (ID_IS(lhs_type_id, ID_TC_SHAPE)) {
        ASSERT1(ID_IS(op->left_id, ID_AST_OP));
        a_operator * l_op = lookup(op->left_id);
        ID l_l_type = ast_get_type_of(l_op->left_id);
        ASSERT1(!ID_IS_INVALID(l_l_type));

        arena_grow(&args_type->types, args.children.size + 1);
        ARENA_APPEND(&args_type->types, l_l_type);
    } else {
        arena_grow(&args_type->types, args.children.size);
    }

    for (size_t i = 0; i < args.children.size; ++i) {
        ARENA_APPEND(&args_type->types, checker_check_expr_node(ARENA_GET(args.children, i, ID), templates));
    }

    Fn_T * caller_type = type_allocate(ID_FN_TYPE);
    caller_type->arg_type = args_type->info.type_id;

    Variable_TC * output_var = tc_allocate(ID_TC_VARIABLE);
    caller_type->ret_type = output_var->variable_id;

    if (ID_IS(lhs_type_id, ID_TC_SHAPE)) {
        Shape_TC * shape = lookup(lhs_type_id);
        lhs_type_id = shape->requirement_id;

        checker_generate_member_function_call_dimension(op, ast_get_interner_id(shape->member_id), shape->requirement_id);
    }

    Constraint_TC * constraint = tc_allocate(ID_TC_CONSTRAINT);
    constraint->from = lhs_type_id;
    constraint->to = caller_type->info.type_id;

    op->type_id = caller_type->ret_type;
    println("ret: {s}", type_to_str(op->type_id));

    return caller_type->ret_type;
}

ID checker_check_op(ID node_id, const Arena templates) {
    a_operator * op = lookup(node_id);

    switch (op->op.key) {
        case CALL: return checker_check_op_call(op, templates);
        case PARENTHESES: return op->type_id = checker_check_expr_node(op->right_id, templates);
        case MEMBER_ACCESS: return checker_check_op_member_access(op, templates);
        case TERNARY:
            ASSERT1(ID_IS(op->right_id, ID_AST_OP));
            if (LOOKUP(op->right_id, a_operator).op.key != TERNARY_BODY) {
                FATAL("Detected an error with the ternary operator. This was most likely caused by operator precedence or nested ternary operators. To remedy this try applying parenthesis around the seperate ternary body entries");
            }
        default: break;
    }

    Variable_TC * result_var = tc_allocate(ID_TC_VARIABLE);

    Fn_T * req_sig = type_allocate(ID_FN_TYPE);
    Tuple_T * fn_args = type_allocate(ID_TUPLE_TYPE);
    ID temp_type_id;

    if (!ID_IS_INVALID(op->left_id)) {
        ASSERT1(op->op.mode == BINARY);
        ARENA_APPEND(&fn_args->types, checker_check_expr_node(op->left_id, templates));
    } else {
        ASSERT1(op->op.mode == UNARY_PRE || op->op.mode == UNARY_POST);
    }


    ARENA_APPEND(&fn_args->types, checker_check_expr_node(op->right_id, templates));

    req_sig->arg_type = fn_args->info.type_id;
    req_sig->ret_type = result_var->variable_id;

    ID name_id = operator_get_intern_id(op->op.key);
    Arena candidates = member_function_index_lookup(name_id);

    ASSERT(candidates.size > 0, "No implementations found for operator: {s}", interner_lookup_str(name_id)._ptr);

    ID call_site_type_id = replace_templates_in_type_with_template_variables(req_sig->info.type_id, templates, 1);
    checker_generate_member_function_call_dimension(op, name_id, call_site_type_id);

    return op->type_id = result_var->variable_id;
}

ID checker_check_expression(ID node_id, const Arena templates) {
    a_expression * expr = lookup(node_id);
    if (expr->children.size == 0) {
        return VOID_TYPE;
    }

    if (expr->children.size == 1) {
        ID child_node_id = ARENA_GET(expr->children, 0, ID);
        ASSERT1(!ID_IS_INVALID(child_node_id));
        return checker_check_expr_node(child_node_id, templates);
    }

    Tuple_T * tuple_type = type_allocate(ID_TUPLE_TYPE);

    for (int i = 0; i < expr->children.size; ++i) {
        ID child_node_id = ARENA_GET(expr->children, i, ID);
        ASSERT1(!ID_IS_INVALID(child_node_id));
        ARENA_APPEND(&tuple_type->types, checker_check_expr_node(child_node_id, templates));
    }

    return tuple_type->info.type_id;
}

ID checker_check_variable(ID node_id, const Arena templates) {
    ASSERT1(ID_IS(node_id, ID_AST_VARIABLE));
    a_variable * variable = lookup(node_id);
    // ASSERT1(id_is_equal(node_id, context_lookup_declaration(variable->name_id)));

    if (!ID_IS(variable->type_id, ID_PLACE_TYPE)) {
        ERROR("Variable \"{s}\" used before declared", interner_lookup_str(variable->name_id)._ptr);
    }

    Place_T * place_type = lookup(variable->type_id);
    if (ID_IS_INVALID(place_type->basetype_id)) {
        Variable_TC * variable_tc = tc_allocate(ID_TC_VARIABLE);
        place_type->basetype_id = variable_tc->variable_id;
    }

    ID type_id = replace_templates_in_type_with_template_variables(variable->type_id, templates, 0);
    return type_id;
}

void checker_check_if(ID node_id, const Arena templates) {
    ID next_id = node_id;
    
    do {
        a_if_statement if_statement = LOOKUP(next_id, a_if_statement);

        if (!ID_IS_INVALID(if_statement.expression_id)) {
            checker_check_expression(if_statement.expression_id, templates);
        } else if (!ID_IS_INVALID(if_statement.next_id)) {
            FATAL("Else is not at the end of an if chain?");
        }

        checker_check_scope(if_statement.body_id, templates);

        if (ID_IS_INVALID(if_statement.next_id)) {
            break;
        }

        next_id = if_statement.next_id;
    } while (!ID_IS_INVALID(next_id));
}

void checker_check_while(ID node_id, const Arena templates) {
    a_while_statement while_statement = LOOKUP(node_id, a_while_statement);

    checker_check_expression(while_statement.expression_id, templates);
    checker_check_scope(while_statement.body_id, templates);
}

void checker_check_for(ID node_id, const Arena templates) {
    a_for_statement for_statement = LOOKUP(node_id, a_for_statement);

    checker_check_expression(for_statement.expression_id, templates);
    checker_check_scope(for_statement.body_id, templates);
}

void checker_check_return(ID node_id, const Arena templates) {
    a_return_statement return_statement = LOOKUP(node_id, a_return_statement);

    ID ret_type = checker_check_expression(return_statement.expression_id, templates);
}

void checker_check_struct(ID node_id) {
    a_structure _struct = LOOKUP(node_id, a_structure);
    context_add_template_list(_struct.templates);
    context_add_declaration_list(_struct.members);

    Arena templates = {.arena = NULL};
    generate_template_constraints(node_id, &templates);

    for (size_t i = 0; i < _struct.declarations.size; ++i) {
        ID child_node_id = ARENA_GET(_struct.declarations, i, ID);
        checker_check_declaration(child_node_id, templates);
    }

    for (size_t i = 0; i < _struct.members.size; ++i) {
        ID child_node_id = ARENA_GET(_struct.members, i, ID);

        switch (child_node_id.type) {
            case ID_AST_SYMBOL: {
                checker_check_symbol(child_node_id, templates);
            } break;
            case ID_AST_FUNCTION: {
                // a_function function = LOOKUP(child_node_id, a_function);
                checker_check_function(child_node_id, &templates);
            } break;
            default:
                ERROR("Invalid ID type: {s}", id_type_to_string(child_node_id.type));
                exit(1);
        }

        // println("child: {s}", ast_to_string(child_node_id));
    }

    context_remove_declaration_list(_struct.members);
    context_remove_template_list(_struct.templates);
}

void checker_check_impl(ID node_id) { }

void checker_check_trait(ID node_id) { }

void checker_check_scope(ID node_id, const Arena templates) {
    a_scope scope = LOOKUP(node_id, a_scope);
    context_add_declaration_list(scope.declarations);

    for (int i = 0; i < scope.nodes.size; ++i) {
        ID child_node_id = ARENA_GET(scope.nodes, i, ID);
        switch (child_node_id.type) {
            case ID_AST_EXPR:
                checker_check_expression(child_node_id, templates);
                break;
            case ID_AST_DECLARATION:
                checker_check_declaration(child_node_id, templates);
                break;
            case ID_AST_IF:
                checker_check_if(child_node_id, templates);
                break;
            case ID_AST_WHILE:
                checker_check_while(child_node_id, templates);
                break;
            case ID_AST_FOR:
                checker_check_for(child_node_id, templates);
                break;
            case ID_AST_RETURN:
                checker_check_return(child_node_id, templates);
                break;
            default:
                ERROR("Invalid scope node type: {s}\n", id_type_to_string(child_node_id.type));
                break;
        }

        // print_ast_tree(child_node_id);
    }

    context_remove_declaration_list(scope.declarations);
}

void checker_check_declaration(ID node_id, const Arena templates) {
    a_declaration declaration = LOOKUP(node_id, a_declaration);
    a_expression * expr = lookup(declaration.expression_id);

    for (size_t i = 0; i < expr->children.size; ++i) {
        ID child_node_id = ARENA_GET(expr->children, i, ID);
        ID symbol_id = INVALID_ID;

        switch (child_node_id.type) {
            case ID_AST_OP: {
                a_operator assignment_op = LOOKUP(child_node_id, a_operator);

                if (assignment_op.op.key != ASSIGNMENT || !ID_IS(assignment_op.left_id, ID_AST_SYMBOL)) {
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
        place_type->is_mut = 1; // Initialize as MutPlace

        variable->type_id = place_type->info.type_id;
    }

    checker_check_expression(declaration.expression_id, templates);

    for (int i = 0; i < expr->children.size; ++i) {
        ID child_node_id = ARENA_GET(expr->children, i, ID);
        a_variable * variable = NULL;

        if (ID_IS(child_node_id, ID_AST_OP)) {
            a_operator assignment_op = LOOKUP(child_node_id, a_operator);
            ASSERT1(ID_IS(assignment_op.left_id, ID_AST_SYMBOL));
            variable = lookup(LOOKUP(assignment_op.left_id, a_symbol).node_id);
        } else {
            ASSERT1(ID_IS(child_node_id, ID_AST_SYMBOL));
            a_symbol symbol = LOOKUP(child_node_id, a_symbol);
            ASSERT1(ID_IS(symbol.node_id, ID_AST_VARIABLE));
            variable = lookup(symbol.node_id);

            if (ID_IS_INVALID(variable->type_id)) {
                FATAL("Empty variable declaration must include a type");
            }
        }

        ASSERT1(variable != NULL);
        ASSERT1(ID_IS(variable->type_id, ID_PLACE_TYPE));
        Place_T place = LOOKUP(variable->type_id, Place_T);

        if (!declaration.is_mut) {
            Place_T * non_mut_place = type_allocate(ID_PLACE_TYPE);
            non_mut_place->basetype_id = place.basetype_id;
            non_mut_place->is_mut = 0;

            variable->type_id = non_mut_place->info.type_id;
        }

        checker_check_variable(variable->info.node_id, templates);
    }
}

void checker_check_function(ID node_id, Arena * templates_ref) {
    a_function function = LOOKUP(node_id, a_function);
    Arena templates = { .arena = NULL };
    if (templates_ref != NULL) {
        templates = *templates_ref;
    }

    generate_template_constraints(node_id, &templates);
    context_add_declaration_list(function.arguments);

    for (int i = 0; i < function.arguments.size; ++i) {
        ID child_node_id = ARENA_GET(function.arguments, i, ID);
        ASSERT(ID_IS(child_node_id, ID_AST_SYMBOL), "Function arguments currently only support declarations");
        a_symbol symbol = LOOKUP(child_node_id, a_symbol);
        checker_check_variable(symbol.node_id, templates);
    }

    checker_check_scope(function.body_id, templates);

    if  (templates_ref != NULL) {
        templates.size = templates_ref->size;
        *templates_ref = templates;
    }

    context_remove_declaration_list(function.arguments);
}

void checker_check_import(ID node_id) {

}

void checker_check_definitions(ID node_id) {
    ASSERT1(!ID_IS_INVALID(node_id));

    switch (node_id.type) {
        case ID_AST_FUNCTION:
            checker_check_function(node_id, NULL); break;
        case ID_AST_DECLARATION:
            checker_check_declaration(node_id, (Arena) {.arena = NULL}); break;
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
    ID module_id;

    checker_check_module(root.entry_point);
    return;

    // println("entry: {s}", root.entry_point);
    kh_foreach_value(&root.modules, module_id, {
        checker_check_module(module_id);
    });

    // perform main function lookup on root.entry_point module
}
