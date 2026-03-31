#include "checker/typing/synthesis.h"

#include "parser/AST.h"
#include "tables/registry_manager.h"
#include "tables/member_functions.h"
#include "checker/typing/checking.h"

ID synthesis_operator_call(Solver * solver, a_operator * op) {
	ASSERT1(ID_IS(op->info.node_id, ID_AST_OP) && op->op.key == ASSIGNMENT);
	FATAL("Unimplemented");
}

ID synthesis_symbol(Solver * solver, ID node_id) {
	ASSERT1(ID_IS(node_id, ID_AST_SYMBOL));

	TermVar * term_var = solver_allocate(solver, ID_TERM_VAR);
	term_var->symbol_id = node_id;
	term_var->type = ((Existential *) solver_allocate(solver, ID_EXISTENIAL))->info.id;

	return term_var->type;
}

ID synthesis_literal(Solver * solver, ID node_id) {
	ASSERT1(ID_IS(node_id, ID_AST_LITERAL));
	a_literal literal = LOOKUP(node_id, a_literal);
	ASSERT1(!ID_IS_INVALID(literal.type_id));
	return literal.type_id;
}

ID synthesis_apply(Solver * solver, ID function_id, ID expr_id) {
	ASSERT1(ID_IS(function_id, ID_AST_FUNCTION));
	a_function function = LOOKUP(function_id, a_function);

	solver_collect_templates(solver, function_id);
	ID fixed_fn_type_id = solver_replace_templates(function.type);
	Fn_T fn_type = LOOKUP(fixed_fn_type_id, Fn_T);

	check(solver, expr_id, fn_type.arg_type);

	return fn_type.ret_type;
}

ID synthesis_operator(Solver * solver, ID node_id) {
	ASSERT1(ID_IS(node_id, ID_AST_OP));
	a_operator * op = lookup(node_id);

	println("Synthesis type of operator: {s}", operator_get_runtime_name(op->op.key));
	switch (op->op.key) {
		case CALL: return synthesis_operator_call(solver, op);
		case PARENTHESES: return op->type_id = synthesis(solver, op->right_id);
		default: break;
	}

	ID name_id = operator_get_intern_id(op->op.key);
	Arena candidates = member_function_index_lookup(name_id);

	ASSERT(candidates.size > 0, "No implementations found for operator: {s}", interner_lookup_str(name_id)._ptr);
	ASSERT(candidates.size == 1, "Use something else for now");
	Marker * marker = solver_allocate(solver, ID_MARKER);

	a_expression * expression = ast_allocate(ID_AST_EXPR, op->info.scope_id);
	expression->children = arena_init(sizeof(ID));

	if (op->op.mode == BINARY) {
		arena_grow(&expression->children, 2);

		ASSERT1(!ID_IS_INVALID(op->left_id));
		ARENA_APPEND(&expression->children, op->left_id);

		ASSERT1(!ID_IS_INVALID(op->right_id));
		ARENA_APPEND(&expression->children, op->right_id);
	} else {
		arena_grow(&expression->children, 1);

		ASSERT1(!ID_IS_INVALID(op->right_id));
		ARENA_APPEND(&expression->children, op->right_id);
	}

	for (size_t i = 0; i < candidates.size; ++i) {
		ID candidate_id = ARENA_GET(candidates, i, ID);
		println("{u}) {s}", i + 1, ast_to_string(candidate_id));
		ID ret_type = synthesis_apply(solver, candidate_id, expression->info.node_id);
		println("\t{s}", type_to_str(ret_type));
	}

	FATAL("Unimplemented");
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

ID synthesis_if(Solver * solver, ID node_id) {
	ASSERT1(ID_IS(node_id, ID_AST_IF));
	ID if_statement_id = node_id;
	a_if_statement if_statement = LOOKUP(node_id, a_if_statement);

	check(solver, if_statement.expression_id, get_bool_type());
	ID if_type = synthesis(solver, if_statement.body_id);

	while (if_statement_id = if_statement.next_id, !ID_IS_INVALID(if_statement_id)) {
		check(solver, if_statement.expression_id, get_bool_type());
		check(solver, if_statement.body_id, if_type);
	}

	return if_type;
}

ID synthesis(Solver * solver, ID node_id) {
	switch (node_id.type) {
		case ID_AST_EXPR: return synthesis_expression(solver, node_id);
		case ID_AST_OP: return synthesis_operator(solver, node_id);
		case ID_AST_IF: return synthesis_if(solver, node_id);
		case ID_AST_SYMBOL: return synthesis_symbol(solver, node_id);
		case ID_AST_LITERAL: return synthesis_literal(solver, node_id);
		default:
			FATAL("Unimplemented AST type \"{s}\"", id_type_to_string(node_id.type));
	}
}
