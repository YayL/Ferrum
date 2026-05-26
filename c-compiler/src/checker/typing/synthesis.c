#include "checker/typing/synthesis.h"

#include "parser/AST.h"
#include "tables/registry_manager.h"
#include "tables/member_functions.h"
#include "checker/symbol.h"
#include "checker/typing/checking.h"
#include "checker/typing/subtyping.h"
#include "checker/typing/utils.h"

ID synthesis_symbol(Solver * solver, ID node_id) {
	ASSERT1(ID_IS(node_id, ID_AST_SYMBOL));
	a_symbol * symbol = lookup(node_id);
	
	if (ID_IS_INVALID(symbol->node_id) && ID_IS_INVALID(qualify_symbol(symbol, ID_AST_DECLARATION))) {
		ERROR("Unable to find symbol: {s}", ast_to_string(node_id));
		exit(1);
	}

	ASSERT1(!ID_IS_INVALID(symbol->node_id));

	ID * type_id_ref = NULL;
	switch (symbol->node_id.type) {
		case ID_AST_VARIABLE: type_id_ref = &(&LOOKUP(symbol->node_id, a_variable))->type_id; break;
		case ID_AST_FUNCTION: type_id_ref = &(&LOOKUP(symbol->node_id, a_function))->type; break;
		default: FATAL("Unimplemented type: {s}", id_type_to_string(symbol->node_id.type));
	}

	if (!ID_IS_INVALID(*type_id_ref)) {
		return solver_replace_templates(*type_id_ref);
	}

	TermVar * term_var = solver_allocate(solver, ID_TERM_VAR);
	term_var->symbol_id = node_id;
	*type_id_ref = term_var->type = ((Existential *) solver_allocate(solver, ID_EXISTENIAL))->info.id;

	return term_var->type;
}

ID synthesis_literal(Solver * solver, ID node_id) {
	ASSERT1(ID_IS(node_id, ID_AST_LITERAL));
	a_literal literal = LOOKUP(node_id, a_literal);
	ASSERT1(!ID_IS_INVALID(literal.type_id));
	return literal.type_id;
}

ID synthesis_operator_member_access(Solver * solver, a_operator * op) {
	ASSERT1(ID_IS(op->info.node_id, ID_AST_OP));

	if (!ID_IS(op->right_id, ID_AST_SYMBOL)) {
		FATAL("Member access must be retrieving a symbol");
	}

	a_symbol * right_member_symbol = lookup(op->right_id);

	if (right_member_symbol->name_ids.size != 1) {
		FATAL("Invalid member access: {s}", interner_lookup_str(right_member_symbol->name_id)._ptr);
	}

	ID member_name_id = right_member_symbol->name_id;
	ID left_type_id = synthesis(solver, op->left_id);

	print_ast_tree(op->info.node_id);
	ID symbol_type_id = INVALID_ID;
	switch (left_type_id.type) {
		case ID_PLACE_TYPE: symbol_type_id = LOOKUP(left_type_id, Place_T).basetype_id; break;
		case ID_REF_TYPE: symbol_type_id = LOOKUP(left_type_id, Ref_T).basetype_id; break;
		default: break;
	}
	
	if (!ID_IS(symbol_type_id, ID_SYMBOL_TYPE)) {
		return INVALID_ID;
	}

	println("Symbol type: {s}", type_to_str(symbol_type_id));
	Symbol_T symbol_type = LOOKUP(symbol_type_id, Symbol_T);
	a_symbol symbol = LOOKUP(symbol_type.symbol_id, a_symbol);
	ASSERT1(!ID_IS_INVALID(symbol.node_id));

	println("{s}", ast_to_string(symbol.node_id));
	Arena declarations = {0};
	switch (symbol.node_id.type) {
		case ID_AST_STRUCT: declarations = LOOKUP(symbol.node_id, a_structure).members; break;
		default: ERROR("Unimplemented: {s}", id_type_to_string(symbol.node_id.type)); exit(1);
	}

	ASSERT1(declarations.size > 0);
	right_member_symbol->node_id = qualify_declaration(declarations, member_name_id);

	return ast_get_type_of(right_member_symbol->node_id);
}

ID synthesis_apply(Solver * solver, ID function_id, ID expr_type_id) {
	ASSERT1(ID_IS(function_id, ID_AST_FUNCTION));
	a_function function = LOOKUP(function_id, a_function);

	// Instantiate
	Marker * marker = solver_allocate(solver, ID_MARKER);
	// println("1");
	solver_collect_templates(solver, function_id, 1);
	// println("2");
	ID fixed_fn_type_id = solver_replace_templates(function.type);
	Fn_T * fn_type = lookup(fixed_fn_type_id);

	// Unify
	// println("3");
	if (!is_subtype(solver, expr_type_id, fn_type->arg_type)) {
		solver_reset_to_marker(solver, marker->info.id);
		return INVALID_ID;
	}

	if (!is_free_from_existentials(fixed_fn_type_id)) {
		DEBUG("Function application still has existential: {s}{s}: {s}", interner_lookup_str(function.name_id)._ptr, type_to_str(expr_type_id), type_to_str(solver_deflate_type(fixed_fn_type_id)));
	}

	// Verify
	// println("4");
	if (!solver_validate_where_clauses(solver, function_id)) {
		solver_reset_to_marker(solver, marker->info.id);
		return INVALID_ID;
	}

	// Return
	// println("5");
	ID deflated_type_id = solver_deflate_type(fn_type->ret_type);
	// println("type: {s}", type_to_str(solver_deflate_type(fixed_fn_type_id)));
	// println("return: {s}", type_to_str(deflated_type_id));
	solver_reset_to_marker(solver, marker->info.id);

	// println("6");
	return deflated_type_id;
}

ID synthesis_operator_call_direct(Solver * solver, a_operator * op) {
	ASSERT1(ID_IS(op->left_id, ID_AST_SYMBOL));

	ID call_site_argument_type_id = synthesis(solver, op->right_id);
	// print_ast_tree(op->info.node_id);
	// println("{s}", type_to_str(call_site_argument_type_id));

	if (!is_free_from_existentials(call_site_argument_type_id)) {
		ERROR("Has existential at call");
	}

	ID called_function_type = synthesis(solver, op->left_id);
	// println("Called function: {s}", type_to_str(called_function_type));

	a_symbol * symbol = lookup(op->left_id);
	qualify_symbol(symbol, ID_AST_DECLARATION);
	ID function_id = symbol->node_id;
	ASSERT1(!ID_IS_INVALID(function_id));
	ASSERT1(ID_IS(function_id, ID_AST_FUNCTION));
	ASSERT1(ID_IS(called_function_type, ID_FN_TYPE));
	return synthesis_apply(solver, function_id, synthesis(solver, op->right_id));
}

ID synthesis_operator_call_member(Solver * solver, a_operator * op, a_operator * dot_op) {
	ASSERT1(ID_IS(op->left_id, ID_AST_OP));
	ASSERT1(dot_op->op.key == MEMBER_ACCESS);

	ASSERT1(ID_IS(dot_op->right_id, ID_AST_SYMBOL));
	a_symbol * member_name_symbol = lookup(dot_op->right_id);
	ASSERT1(member_name_symbol->name_ids.size == 1);
	ID member_name_id = member_name_symbol->name_id;
	op->left_id = dot_op->right_id;

	ASSERT1(ID_IS(op->right_id, ID_AST_EXPR));
	a_expression * call_arguments = lookup(op->right_id);

	// Add one slot and move everything over by 1
	call_arguments->children.size += 1;
	arena_grow(&call_arguments->children, call_arguments->children.size);
	for (int i = call_arguments->children.size - 2; i >= 0; --i) {
		ARENA_GET(call_arguments->children, i + 1, ID) = ARENA_GET(call_arguments->children, i, ID);
	}

	// Add left of member access as first argument
	ARENA_GET(call_arguments->children, 0, ID) = dot_op->left_id;

	Tuple_T * tuple_type = type_allocate(ID_TUPLE_TYPE);
	arena_grow(&tuple_type->types, call_arguments->children.size);

	for (size_t i = 0; i < call_arguments->children.size; ++i) {
		ARENA_APPEND(&tuple_type->types, synthesis(solver, ARENA_GET(call_arguments->children, i, ID)));
	}

	ID lhs_type_id = ast_get_type_of(dot_op->left_id);

	ID member_of_type_id = INVALID_ID;
	switch (lhs_type_id.type) {
		case ID_INVALID_TYPE: FATAL("Invalid");
		case ID_PLACE_TYPE: member_of_type_id = LOOKUP(lhs_type_id, Place_T).basetype_id; break;
		case ID_REF_TYPE: member_of_type_id = LOOKUP(lhs_type_id, Ref_T).basetype_id; break;
		default: break;
	}

	if (!ID_IS_INVALID(member_of_type_id)) {
		ASSERT1(ID_IS(member_of_type_id, ID_SYMBOL_TYPE));
		Symbol_T symbol_type = LOOKUP(member_of_type_id, Symbol_T);
		a_symbol symbol = LOOKUP(symbol_type.symbol_id, a_symbol);

		Arena declarations = {0};
		switch (symbol.node_id.type) {
			case ID_AST_STRUCT: declarations = LOOKUP(symbol.node_id, a_structure).members; break;
			default: FATAL("Unimplemented: {s}", id_type_to_string(symbol.node_id.type));
		}

		ASSERT1(declarations.size != 0);

		ASSERT1(ID_IS_INVALID(member_name_symbol->node_id));
		ID found_member_id = qualify_declaration(declarations, member_name_id);
		ASSERT1(ID_IS(found_member_id, ID_AST_FUNCTION));

		if (!ID_IS_INVALID(found_member_id)) {
			ID res = synthesis_apply(solver, found_member_id, tuple_type->info.type_id);
			if (!ID_IS_INVALID(res)) {
				member_name_symbol->node_id = found_member_id;
				return res;
			}
		}
	}

	print_ast_tree(op->info.node_id);
	// println("res: {s}", type_to_str(call_argument_type_id));

	ERROR("Unimplemented");
	exit(1);
}

ID synthesis_operator_call(Solver * solver, a_operator * op) {
	ASSERT1(ID_IS(op->info.node_id, ID_AST_OP));

	ID call_return_type_id = INVALID_ID;

	switch (op->left_id.type) {
		case ID_AST_SYMBOL: call_return_type_id = synthesis_operator_call_direct(solver, op); break;
		case ID_AST_OP: {
			a_operator * left_op = lookup(op->left_id);
			if (left_op->op.key != MEMBER_ACCESS) {
				FATAL("Invalid operator in function call");
			}

			call_return_type_id = synthesis_operator_call_member(solver, op, left_op); break;
		}
		default: FATAL("Unimplemented: {s}", id_type_to_string(op->left_id.type));
	}

	if (ID_IS_INVALID(call_return_type_id)) {
		ERROR("Unable to find valid function to call: {s}", ast_to_string(op->info.node_id));
	}

	return call_return_type_id;
}

ID synthesis_operator(Solver * solver, ID node_id) {
	ASSERT1(ID_IS(node_id, ID_AST_OP));
	a_operator * op = lookup(node_id);

	if (!ID_IS_INVALID(op->type_id)) {
		op->type_id = solver_replace_templates(op->type_id);
	}

	// println("Synthesis type of operator: {s}", operator_get_runtime_name(op->op.key));
	// These operators are not overloadable
	switch (op->op.key) {
		case CALL: return op->type_id = solver_deflate_type(synthesis_operator_call(solver, op));
		case MEMBER_ACCESS: return op->type_id = solver_deflate_type(synthesis_operator_member_access(solver, op));
		case PARENTHESES: return op->type_id = solver_deflate_type(synthesis(solver, op->right_id));
		default: break;
	}

	ID name_id = operator_get_intern_id(op->op.key);
	Arena candidates = member_function_index_lookup(name_id);

	ASSERT(candidates.size > 0, "No implementations found for operator: {s}", interner_lookup_str(name_id)._ptr);

	Tuple_T * tuple_type = type_allocate(ID_TUPLE_TYPE);

	if (op->op.mode == BINARY) {
		arena_grow(&tuple_type->types, 2);

		ASSERT1(!ID_IS_INVALID(op->left_id));
		ARENA_APPEND(&tuple_type->types, synthesis(solver, op->left_id));

		ASSERT1(!ID_IS_INVALID(op->right_id));
		ARENA_APPEND(&tuple_type->types, synthesis(solver, op->right_id));
	} else {
		arena_grow(&tuple_type->types, 1);

		ASSERT1(!ID_IS_INVALID(op->right_id));
		ARENA_APPEND(&tuple_type->types, synthesis(solver, op->right_id));
	}

	ID ret_type = INVALID_ID;

	if (candidates.size > 1 && !is_free_from_existentials(tuple_type->info.type_id)) {
		FATAL("Unimplemented");
	}

	for (size_t i = 0; i < candidates.size; ++i) {
		ID candidate_id = ARENA_GET(candidates, i, ID);
		ID temp = synthesis_apply(solver, candidate_id, tuple_type->info.type_id);

		if (ID_IS_INVALID(temp)) {
			continue;
		} else if (!ID_IS_INVALID(ret_type)) {
			FATAL("Ambigious: {s} | {s}", type_to_str(ret_type), type_to_str(temp));
		}

		ret_type = temp;
	}

	if (ID_IS_INVALID(ret_type)) {
		ERROR("Unable to find valid operator implementation({u}): {s} | {s}", candidates.size, ast_to_string(node_id), type_to_str(tuple_type->info.type_id));
		return INVALID_ID;
	}

	ID deflated_ret_type_id = solver_deflate_type(ret_type);

	if (ID_IS_INVALID(op->type_id)) {
		return op->type_id = deflated_ret_type_id;
	} else if (!is_subtype(solver, op->type_id, deflated_ret_type_id)) {
		FATAL("Invalid function return type");
	}

	return op->type_id;
}

ID synthesis_expression(Solver * solver, ID node_id) {
	a_expression expr = LOOKUP(node_id, a_expression);

	if (expr.children.size == 1) {
		return synthesis(solver, ARENA_GET(expr.children, 0, ID));
	}

	Tuple_T * tuple_type = type_allocate(ID_TUPLE_TYPE);
	arena_grow(&tuple_type->types, expr.children.size);

	for (size_t i = 0; i < expr.children.size; ++i) {
		ID expr_node = ARENA_GET(expr.children, i, ID);
		ARENA_APPEND(&tuple_type->types, synthesis(solver, expr_node));
	}
	
	return tuple_type->info.type_id;
}

ID synthesis_declaration(Solver * solver, ID node_id) {
	ASSERT1(ID_IS(node_id, ID_AST_DECLARATION));
	a_declaration declaration = LOOKUP(node_id, a_declaration);

	ASSERT1(ID_IS(declaration.expression_id, ID_AST_EXPR));
	a_expression expr = LOOKUP(declaration.expression_id, a_expression);

	for (size_t i = 0; i < expr.children.size; ++i) {
		ID child_id = ARENA_GET(expr.children, i, ID);

		ID expr_type_id = INVALID_ID, symbol_id = INVALID_ID;

		if (ID_IS(child_id, ID_AST_OP)) {
			a_operator op = LOOKUP(child_id, a_operator);
			if (op.op.key != ASSIGNMENT) {
				ERROR("Declaration without assignment");
				continue;
			}

			expr_type_id = synthesis(solver, op.right_id);

			ASSERT1(ID_IS(op.left_id, ID_AST_SYMBOL));
			symbol_id = op.left_id;
		} else if (ID_IS(child_id, ID_AST_SYMBOL)) {
			symbol_id = child_id;
		} else {
			FATAL("Invalid type: {s}", id_type_to_string(child_id.type));
		}

		ASSERT1(ID_IS(symbol_id, ID_AST_SYMBOL));
		a_symbol symbol = LOOKUP(symbol_id, a_symbol);
		ASSERT1(ID_IS(symbol.node_id, ID_AST_VARIABLE));
		a_variable * variable = lookup(symbol.node_id);

		if (ID_IS_INVALID(expr_type_id)) {
			Existential * existential = solver_allocate(solver, ID_EXISTENIAL);
			expr_type_id = existential->info.id;
		}

		if (!ID_IS_INVALID(variable->type_id) && !is_subtype(solver, expr_type_id, variable->type_id)) {
			ERROR("Type Error: {s} <: {s}", type_to_str(expr_type_id), type_to_str(variable->type_id));
		}

		ID deflated_type_id = solver_deflate_type(expr_type_id);
		if (ID_IS(deflated_type_id, ID_PLACE_TYPE)) {
			deflated_type_id = LOOKUP(deflated_type_id, Place_T).basetype_id;
		}

		Place_T * place = type_allocate(ID_PLACE_TYPE);
		place->is_mut = declaration.is_mut;
		place->basetype_id = deflated_type_id;
		variable->type_id = place->info.type_id;
	}

	return VOID_TYPE;
}

ID synthesis_if(Solver * solver, ID node_id) {
	ASSERT1(ID_IS(node_id, ID_AST_IF));
	ID if_statement_id = node_id;
	a_if_statement if_statement = LOOKUP(node_id, a_if_statement);

	const ID bool_type_id = get_bool_type();

	if (!check(solver, if_statement.expression_id, bool_type_id)) {
		ERROR("If statement expression is not bool");
	}

	ID if_type_id = synthesis(solver, if_statement.body_id);

	while (if_statement_id = if_statement.next_id, !ID_IS_INVALID(if_statement_id)) {
		if_statement = LOOKUP(if_statement_id, a_if_statement);
		if (!check(solver, if_statement.expression_id, bool_type_id)) {
			ERROR("If statement expression is not bool");
		}

		if (!check(solver, if_statement.body_id, if_type_id)) {
			ERROR("Chained if statements do not match return type");
		}
	}

	return if_type_id;
}

ID synthesis_for(Solver * solver, ID node_id) {
	ASSERT1(ID_IS(node_id, ID_AST_FOR));
	a_for_statement for_statement = LOOKUP(node_id, a_for_statement);

	ID expr_type_id = synthesis(solver, for_statement.expression_id);
	synthesis(solver, for_statement.body_id);

	return VOID_TYPE;
}

ID synthesis_scope(Solver * solver, ID node_id) {
	ASSERT1(ID_IS(node_id, ID_AST_SCOPE));
	a_scope scope = LOOKUP(node_id, a_scope);

	if (scope.nodes.size == 0) {
		return VOID_TYPE;
	}

	for (size_t i = 0; i < scope.nodes.size - 1; ++i) {
		ID statement_id = ARENA_GET(scope.nodes, i, ID);
		synthesis(solver, statement_id);
	}

	ASSERT1(scope.nodes.size != 0);
	ID return_statement_id = ARENA_GET(scope.nodes, scope.nodes.size - 1, ID);
	switch (return_statement_id.type) {
		case ID_AST_RETURN: return synthesis(solver, LOOKUP(return_statement_id, a_return_statement).expression_id);
		case ID_AST_EXPR: return synthesis(solver, return_statement_id);
		default: ERROR("Missing return in scope");
	}

	return INVALID_ID;
}

ID synthesis(Solver * solver, ID node_id) {
	ID res_type_id = INVALID_ID;
	switch (node_id.type) {
		case ID_AST_EXPR: res_type_id = synthesis_expression(solver, node_id); break;
		case ID_AST_DECLARATION: res_type_id = synthesis_declaration(solver, node_id); break;
		case ID_AST_OP: res_type_id = synthesis_operator(solver, node_id); break;
		case ID_AST_IF: res_type_id = synthesis_if(solver, node_id); break;
		case ID_AST_SYMBOL: res_type_id = synthesis_symbol(solver, node_id); break;
		case ID_AST_LITERAL: res_type_id = synthesis_literal(solver, node_id); break;
		case ID_AST_SCOPE: res_type_id = synthesis_scope(solver, node_id); break;
		case ID_AST_FOR: res_type_id = synthesis_for(solver, node_id); break;
		default:
			FATAL("Unimplemented AST type \"{s}\"", id_type_to_string(node_id.type));
	}

	if (ID_IS_INVALID(res_type_id)) {
		ERROR("Had to fill in INVALID synthesis with existential");
		res_type_id = ((Existential *) solver_allocate(solver, ID_EXISTENIAL))->info.id;
	}

	return res_type_id;
}
