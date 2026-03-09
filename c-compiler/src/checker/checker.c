#include "checker/checker.h"

#include "parser/AST.h"
#include "checker/context.h"
#include "checker/symbol.h"
#include "tables/registry_manager.h"
#include "tables/member_functions.h"
#include "checker/typing/gathering.h"
#include "checker/typing/solver.h"
#include "checker/typing/groups.h"

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
                if (ID_IS_EQUAL(member_name_id, ast_get_interner_id(ARENA_GET(_struct.members, i, ID)))) {
                    return 1;
                }
            }
        } break;
        case ID_AST_IMPL: FATAL("Not implemented");
        default: return 0;
    }

    return 0;
}

static inline ID checker_generate_implicit_cast(Solver * solver, ID type_id, const Arena templates) {
	if (ID_IS(type_id, ID_TC_VARIABLE)) {
		return type_id;
	}

	Arena * candidates = context_find_implicit_casts(type_id);

	if (candidates == NULL) {
		return type_id;
	}

	Dimension_TC * dimension = tc_allocate(ID_TC_DIMENSION);
	dimension->candidates = *candidates;
    dimension_init_choices(&solver->resolver, dimension, NULL);

	Variable_TC * variable = tc_allocate(ID_TC_VARIABLE);

	Cast_TC * cast = tc_allocate(ID_TC_CAST);
	cast->dimension_id = dimension->dimension_id;
	cast->variable_id = variable->variable_id;

	solver_unify(solver, type_id, cast->cast_id, NULL);

	return variable->variable_id;
}

ID checker_check_expr_node(Solver * solver, ID node_id, const Arena templates) {
    ID ret_value;
    switch (node_id.type) {
        case ID_AST_OP:
            ret_value = checker_check_op(solver, node_id, templates); break;
        case ID_AST_VARIABLE:
            ret_value = checker_check_variable(solver, node_id, templates); break;
        case ID_AST_EXPR:
            ret_value = checker_check_expression(solver, node_id, templates); break;
        case ID_AST_LITERAL:
            ret_value = checker_check_literal(solver, node_id); break;
        case ID_AST_SYMBOL:
            ret_value = checker_check_symbol(solver, node_id, templates); break;
        default:
            FATAL("Unimplemented expr node type: {s}", id_type_to_string(node_id.type));
    }

    // println("Expr node({s}): {s}", id_type_to_string(node_id.type), ast_to_string(ret_value));
    return checker_generate_implicit_cast(solver, ret_value, templates);
}

ID checker_check_literal(Solver * solver, ID node_id) {
    ASSERT1(ID_IS(node_id, ID_AST_LITERAL));

    a_literal literal = LOOKUP(node_id, a_literal);
    ASSERT1(!ID_IS_INVALID(literal.type_id));
    ASSERT1(literal.value._ptr != NULL && literal.value.length > 0)

    return literal.type_id;
}

ID checker_check_symbol(Solver * solver, ID node_id, const Arena templates) {
    ASSERT1(ID_IS(node_id, ID_AST_SYMBOL));
    a_symbol * symbol = lookup(node_id);

    if (!ID_IS_INVALID(symbol->node_id)) {
        return checker_check_expr_node(solver, symbol->node_id, templates);
    }

    if (symbol->name_ids.size > 1) {
        symbol->node_id = qualify_symbol(symbol, ID_AST_DECLARATION);
    } else {
        symbol->node_id = context_lookup_declaration(symbol->name_id);
    }

    if (ID_IS_EQUAL(symbol->info.scope_id, ast_get_scope_id(symbol->node_id))) {
        return replace_templates_in_type_with_template_variables(solver, ast_get_type_of(symbol->node_id), templates);
    }

    switch (symbol->node_id.type) {
        case ID_AST_FUNCTION: {
            Arena temp_templates = {0};
            generate_template_constraints(solver, symbol->node_id, &temp_templates, NULL);
            return replace_templates_in_type_with_template_variables(solver, ast_get_type_of(symbol->node_id), temp_templates);
        } break;
        case ID_AST_VARIABLE: {
            Arena temp_templates = {0};
            generate_template_constraints(solver, symbol->node_id, &temp_templates, NULL);
            ID type_id = replace_templates_in_type_with_template_variables(solver, ast_get_type_of(symbol->node_id), temp_templates);
            return type_id;
        }
        case ID_INVALID_TYPE: FATAL("Unable to find symbol: {s}", interner_lookup_str(symbol->name_id)._ptr);
        default: break;
    }

    return ast_get_type_of(symbol->node_id);
}

void checker_generate_member_function_call_dimension(Solver * solver, a_operator * op, ID member_name_id, ID function_signature) {
    Arena candidates = member_function_index_lookup(member_name_id);
    
    if (candidates.size == 0) {
        ERROR("Unable to find member function \"{s}\"", interner_lookup_str(member_name_id)._ptr);
        return;
    } 

    Dimension_TC * dimension = tc_allocate(ID_TC_DIMENSION);
    dimension->candidates = candidates;
    dimension_init_choices(&solver->resolver, dimension, NULL);

    op->dimension_id = dimension->dimension_id;

    solver_unify(solver, function_signature, dimension->dimension_id, NULL);
}

ID checker_check_op_member_access(Solver * solver, a_operator * op, const Arena templates) {
    FATAL("Not implemented");
    ID lhs_type_id = checker_check_expr_node(solver, op->left_id, templates);

    Variable_TC * result_var = tc_allocate(ID_TC_VARIABLE);
    Shape_TC * shape = tc_allocate(ID_TC_SHAPE);

    ASSERT1(ID_IS(op->right_id, ID_AST_SYMBOL));
    shape->member_id = op->right_id;
    shape->requirement_id = result_var->variable_id;

    // Constraint_TC * constraint = tc_allocate(ID_TC_CONSTRAINT);
    // constraint->from = lhs_type_id;
    // constraint->to = shape->shape_id;

    op->type_id = lhs_type_id;

    return shape->shape_id;
}

ID checker_check_op_call(Solver * solver, a_operator * op, const Arena templates) {
    FATAL("Not implemented");
    ASSERT1(!ID_IS_INVALID(op->left_id));
    // Signature to match against
    ID lhs_type_id = checker_check_expr_node(solver, op->left_id, templates);

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
        ARENA_APPEND(&args_type->types, checker_check_expr_node(solver, ARENA_GET(args.children, i, ID), templates));
    }

    Fn_T * caller_type = type_allocate(ID_FN_TYPE);
    caller_type->arg_type = args_type->info.type_id;

    Variable_TC * output_var = tc_allocate(ID_TC_VARIABLE);
    caller_type->ret_type = output_var->variable_id;

    if (ID_IS(lhs_type_id, ID_TC_SHAPE)) {
        Shape_TC * shape = lookup(lhs_type_id);
        lhs_type_id = shape->requirement_id;

        checker_generate_member_function_call_dimension(solver, op, ast_get_interner_id(shape->member_id), shape->requirement_id);
    }

    // Constraint_TC * constraint = tc_allocate(ID_TC_CONSTRAINT);
    // constraint->from = lhs_type_id;
    // constraint->to = caller_type->info.type_id;

    op->type_id = caller_type->ret_type;
    println("ret: {s}", type_to_str(op->type_id));

    return caller_type->ret_type;
}

ID checker_check_op(Solver * solver, ID node_id, const Arena templates) {
    a_operator * op = lookup(node_id);

    switch (op->op.key) {
        case CALL: return checker_check_op_call(solver, op, templates);
        case PARENTHESES: return op->type_id = checker_check_expr_node(solver, op->right_id, templates);
        case MEMBER_ACCESS: return checker_check_op_member_access(solver, op, templates);
        case TERNARY:
            ASSERT1(ID_IS(op->right_id, ID_AST_OP));
            if (LOOKUP(op->right_id, a_operator).op.key != TERNARY_BODY) {
                FATAL("Detected an error with the ternary operator. This was most likely caused by operator precedence or nested ternary operators. To remedy this try applying parenthesis around the seperate ternary body entries");
            }
        default: break;
    }

    Fn_T * req_sig = type_allocate(ID_FN_TYPE);
    Tuple_T * fn_args = type_allocate(ID_TUPLE_TYPE);
    ID temp_type_id;

    if (!ID_IS_INVALID(op->left_id)) {
        ASSERT1(op->op.mode == BINARY);
        ARENA_APPEND(&fn_args->types, checker_check_expr_node(solver, op->left_id, templates));
    } else {
        ASSERT1(op->op.mode == UNARY_PRE || op->op.mode == UNARY_POST);
    }

    ARENA_APPEND(&fn_args->types, checker_check_expr_node(solver, op->right_id, templates));

    req_sig->arg_type = fn_args->info.type_id;

    if (ID_IS_INVALID(op->type_id)) {
        Variable_TC * result_var = tc_allocate(ID_TC_VARIABLE);
        req_sig->ret_type = result_var->variable_id;
    } else {
        req_sig->ret_type = op->type_id;
    }

    ID name_id = operator_get_intern_id(op->op.key);
    Arena candidates = member_function_index_lookup(name_id);

    ASSERT(candidates.size > 0, "No implementations found for operator: {s}", interner_lookup_str(name_id)._ptr);

    ID call_site_type_id = replace_templates_in_type_with_template_variables(solver, req_sig->info.type_id, templates);
    checker_generate_member_function_call_dimension(solver, op, name_id, call_site_type_id);

    return op->type_id = req_sig->ret_type;
}

ID checker_check_expression(Solver * solver, ID node_id, const Arena templates) {
    a_expression * expr = lookup(node_id);
    if (expr->children.size == 0) {
        return expr->type_id = VOID_TYPE;
    }

    if (expr->children.size == 1) {
        ID child_node_id = ARENA_GET(expr->children, 0, ID);
        ASSERT1(!ID_IS_INVALID(child_node_id));
        return expr->type_id = checker_check_expr_node(solver, child_node_id, templates);
    }

    Tuple_T * tuple_type = type_allocate(ID_TUPLE_TYPE);

    for (int i = 0; i < expr->children.size; ++i) {
        ID child_node_id = ARENA_GET(expr->children, i, ID);
        ASSERT1(!ID_IS_INVALID(child_node_id));
        ARENA_APPEND(&tuple_type->types, checker_check_expr_node(solver, child_node_id, templates));
    }

    return expr->type_id = tuple_type->info.type_id;
}

ID checker_check_variable(Solver * solver, ID node_id, const Arena templates) {
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

    return replace_templates_in_type_with_template_variables(solver, variable->type_id, templates);
}

void checker_check_if(Solver * solver, ID node_id, const Arena templates) {
    ID next_id = node_id;
    
    do {
        a_if_statement if_statement = LOOKUP(next_id, a_if_statement);

        if (!ID_IS_INVALID(if_statement.expression_id)) {
            checker_check_expression(solver, if_statement.expression_id, templates);
        } else if (!ID_IS_INVALID(if_statement.next_id)) {
            FATAL("Else is not at the end of an if chain?");
        }

        checker_check_scope(solver, if_statement.body_id, templates);

        if (ID_IS_INVALID(if_statement.next_id)) {
            break;
        }

        next_id = if_statement.next_id;
    } while (!ID_IS_INVALID(next_id));
}

void checker_check_while(Solver * solver, ID node_id, const Arena templates) {
    a_while_statement while_statement = LOOKUP(node_id, a_while_statement);

    checker_check_expression(solver, while_statement.expression_id, templates);
    checker_check_scope(solver, while_statement.body_id, templates);
}

void checker_check_for(Solver * solver, ID node_id, const Arena templates) {
    a_for_statement for_statement = LOOKUP(node_id, a_for_statement);

    checker_check_expression(solver, for_statement.expression_id, templates);
    checker_check_scope(solver, for_statement.body_id, templates);
}

void checker_check_return(Solver * solver, ID node_id, const Arena templates) {
    a_return_statement return_statement = LOOKUP(node_id, a_return_statement);

    ID ret_type = checker_check_expression(solver, return_statement.expression_id, templates);
}

void checker_check_struct(Solver * solver, ID node_id) {
    a_structure _struct = LOOKUP(node_id, a_structure);
    context_add_template_list(_struct.templates);
    context_add_declaration_list(_struct.members);

    Arena templates = {.arena = NULL};
    generate_template_constraints(solver, node_id, &templates, NULL);

    for (size_t i = 0; i < _struct.declarations.size; ++i) {
        ID child_node_id = ARENA_GET(_struct.declarations, i, ID);
        checker_check_declaration(solver, child_node_id, templates);
    }

    for (size_t i = 0; i < _struct.members.size; ++i) {
        ID child_node_id = ARENA_GET(_struct.members, i, ID);

        switch (child_node_id.type) {
            case ID_AST_SYMBOL: {
                checker_check_symbol(solver, child_node_id, templates);
            } break;
            case ID_AST_FUNCTION: {
                // a_function function = LOOKUP(child_node_id, a_function);
                checker_check_function(solver, child_node_id, &templates);
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

void checker_check_impl(Solver * solver, ID node_id) { }

void checker_check_trait(Solver * solver, ID node_id) { }

void checker_check_scope(Solver * solver, ID node_id, const Arena templates) {
    a_scope scope = LOOKUP(node_id, a_scope);
    context_add_declaration_list(scope.declarations);

    for (int i = 0; i < scope.nodes.size; ++i) {
        ID child_node_id = ARENA_GET(scope.nodes, i, ID);
        switch (child_node_id.type) {
            case ID_AST_EXPR:
                checker_check_expression(solver, child_node_id, templates);
                break;
            case ID_AST_DECLARATION:
                checker_check_declaration(solver, child_node_id, templates);
                break;
            case ID_AST_IF:
                checker_check_if(solver, child_node_id, templates);
                break;
            case ID_AST_WHILE:
                checker_check_while(solver, child_node_id, templates);
                break;
            case ID_AST_FOR:
                checker_check_for(solver, child_node_id, templates);
                break;
            case ID_AST_RETURN:
                checker_check_return(solver, child_node_id, templates);
                break;
            default:
                ERROR("Invalid scope node type: {s}\n", id_type_to_string(child_node_id.type));
                break;
        }

        // print_ast_tree(child_node_id);
    }

    context_remove_declaration_list(scope.declarations);
}

void checker_check_declaration(Solver * solver, ID node_id, const Arena templates) {
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

    checker_check_expression(solver, declaration.expression_id, templates);

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
    }
}

void checker_check_function(Solver * solver, ID node_id, Arena * templates_ref) {
    a_function function = LOOKUP(node_id, a_function);
    Arena templates = { .arena = NULL };
    if (templates_ref != NULL) {
        templates = *templates_ref;
    }

    generate_template_constraints(solver, node_id, &templates, NULL);
    context_add_declaration_list(function.arguments);

    for (int i = 0; i < function.arguments.size; ++i) {
        ID child_node_id = ARENA_GET(function.arguments, i, ID);
        ASSERT(ID_IS(child_node_id, ID_AST_SYMBOL), "Function arguments currently only support declarations");
        a_symbol symbol = LOOKUP(child_node_id, a_symbol);
        checker_check_variable(solver, symbol.node_id, templates);
    }

    checker_check_scope(solver, function.body_id, templates);

    if  (templates_ref != NULL) {
        templates.size = templates_ref->size;
        *templates_ref = templates;
    }

    // LOOP_OVER_REGISTRY(Group_TC, group, {
    //     if (!ID_IS_INVALID(group->parent_group_id)) {
    //         continue;
    //     }
    //
    //     println("\nGroup {u}:", group->group_id.id);
    //
    //     ID requirement_id = group->requirement;
    //     while (!ID_IS_INVALID(requirement_id)) {
    //         Requirement_TC requirement = LOOKUP(requirement_id, Requirement_TC);
    //         println("{s}", type_to_str(requirement.type_id));
    //         requirement_id = requirement.next_requirement;
    //     }
    // });

    // LOOP_OVER_REGISTRY(Variable_TC, variable, {
    //     if (ID_IS_INVALID(variable->group_id)) { continue; }
    //     println("{s} - Group {u}", type_to_str(variable->variable_id), group_find_root_group(LOOKUP(variable->group_id, Group_TC).group_id)->group_id.id);
    // });

    context_remove_declaration_list(function.arguments);
}

void checker_check_import(Solver * solver, ID node_id) {

}

void checker_check_definitions(Solver * solver, ID node_id) {
    ASSERT1(!ID_IS_INVALID(node_id));

    switch (node_id.type) {
        case ID_AST_FUNCTION:
            checker_check_function(solver, node_id, NULL); break;
        case ID_AST_DECLARATION:
            checker_check_declaration(solver, node_id, (Arena) {.arena = NULL}); break;
        case ID_AST_STRUCT:
            checker_check_struct(solver, node_id); break;
        case ID_AST_TRAIT:
            checker_check_trait(solver, node_id); break;
        case ID_AST_IMPL:
            checker_check_impl(solver, node_id); break;
        case ID_AST_IMPORT:
            checker_check_import(solver, node_id); break;
        default:
            FATAL("Invalid AST type: {s}", id_type_to_string(node_id.type));
    }
}

void checker_check_module(Solver * solver, ID node_id) {
    ASSERT1(ID_IS(node_id, ID_AST_MODULE));
    a_module module = LOOKUP(node_id, a_module);
    context_enter_module(module);

    for (int i = 0; i < module.members.size; ++i) {
        checker_check_definitions(solver, ARENA_GET(module.members, i, ID));
    }

    context_exit_module(module);
}

void checker_check(a_root root) {
    ID module_id;

    struct solver solver;
    solver_initialize(&solver);

    checker_check_module(&solver, root.entry_point);
    return;

    // println("entry: {s}", root.entry_point);
    kh_foreach_value(&root.modules, module_id, {
        checker_check_module(&solver, module_id);
    });

    // perform main function lookup on root.entry_point module
}
