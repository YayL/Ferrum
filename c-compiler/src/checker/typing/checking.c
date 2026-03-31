#include "checker/typing/checking.h"

#include "checker/typing/synthesis.h"
#include "checker/typing/subtyping.h"
#include "tables/registry_manager.h"

void check_type(Solver * solver, ID node_id, ID expected_type) {
	ID A = synthesis(solver, node_id);
}

void check_scope(Solver * solver, ID node_id, ID expected_type) {
	ASSERT1(ID_IS(node_id, ID_AST_SCOPE));
	a_scope scope = LOOKUP(node_id, a_scope);

	for (size_t i = 0; i < scope.nodes.size - 1; ++i) {
		ID statement_id = ARENA_GET(scope.nodes, i, ID);
		check(solver, statement_id, VOID_TYPE);
	}

	if (scope.nodes.size > 0) {
		ID return_statement_id = ARENA_GET(scope.nodes, scope.nodes.size - 1, ID);
		if (ID_IS(node_id, ID_AST_RETURN)) {
			a_return_statement return_statement = LOOKUP(return_statement_id, a_return_statement);
			check(solver, return_statement.expression_id, expected_type);
		} else if (ID_IS(node_id, ID_AST_EXPR)) {
			check(solver, node_id, expected_type);
		} else {
			ERROR("Missing return in scope");
		}
	}
}

void check_declaration(Solver * solver, ID node_id, ID expected_type) {
	ASSERT1(ID_IS(node_id, ID_AST_DECLARATION));
	a_declaration declaration = LOOKUP(node_id, a_declaration);

	ASSERT1(ID_IS(declaration.expression_id, ID_AST_EXPR));
	a_expression expr = LOOKUP(declaration.expression_id, a_expression);

	for (size_t i = 0; i < expr.children.size; ++i) {
		ID child_id = ARENA_GET(expr.children, i, ID);

		if (ID_IS(child_id, ID_AST_OP)) {
			a_operator op = LOOKUP(child_id, a_operator);
			if (op.op.key != ASSIGNMENT) {
				ERROR("Declaration without assignment");
				return;
			}

			ASSERT1(ID_IS(op.left_id, ID_AST_SYMBOL));
			a_symbol symbol = LOOKUP(op.left_id, a_symbol);
			ASSERT1(ID_IS(symbol.node_id, ID_AST_VARIABLE));
			a_variable * variable = lookup(symbol.node_id);

			if (!ID_IS_INVALID(variable->type_id)) {
				TermVar * term_var_pre = solver_allocate(solver, ID_TERM_VAR);
				term_var_pre->symbol_id = symbol.info.node_id;

				Place_T * place_type_pre = type_allocate(ID_PLACE_TYPE);
				place_type_pre->basetype_id = variable->type_id;
				place_type_pre->is_mut = 1;
				term_var_pre->type = place_type_pre->info.type_id;

				check(solver, child_id, variable->type_id);

				TermVar * term_var_post = solver_allocate(solver, ID_TERM_VAR);
				term_var_post->symbol_id = symbol.info.node_id;

				Place_T * place_type_post = type_allocate(ID_PLACE_TYPE);
				place_type_post->basetype_id = variable->type_id;
				place_type_post->is_mut = declaration.is_mut;
				term_var_post->type = place_type_post->info.type_id;

				continue;
			} else {
				TermVar * term_var = solver_allocate(solver, ID_TERM_VAR);
				term_var->symbol_id = symbol.info.node_id;
				term_var->type = variable->type_id;
			}
		}

		ID expr_type = synthesis(solver, child_id);
		println("Expr type: {s}", type_to_str(expr_type));
		exit(0);
	}

}

void check_expression(Solver * solver, ID node_id, ID expected_type) {
	ASSERT1(ID_IS(node_id, ID_AST_EXPR));
	ID expr_type = synthesis(solver, node_id);
	println("expr_type: {s} vs {s}", type_to_str(expr_type), type_to_str(expected_type));
	if (is_subtype(expr_type, expected_type)) {
		println("YES!");
	} else {
		println("NO!");
	}
}

void check(Solver * solver, ID node_id, ID expected_type) {
	switch (node_id.type) {
		case ID_TUPLE_TYPE: check_type(solver, node_id, expected_type); break;
		case ID_AST_SCOPE: check_scope(solver, node_id, expected_type); break;
		case ID_AST_DECLARATION: check_declaration(solver, node_id, expected_type); break;
		case ID_AST_EXPR: check_expression(solver, node_id, expected_type); break;
		// case ID_AST_IF: check_if(solver, node_id, expected_type); break;
		default:
			FATAL("Unimplemented AST type \"{s}\"", id_type_to_string(node_id.type));
	}
}
