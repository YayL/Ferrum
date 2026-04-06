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
		synthesis(solver, statement_id);
	}

	if (scope.nodes.size > 0) {
		ID return_statement_id = ARENA_GET(scope.nodes, scope.nodes.size - 1, ID);
		switch (return_statement_id.type) {
			case ID_AST_RETURN: check(solver, LOOKUP(return_statement_id, a_return_statement).expression_id, expected_type); break;
			case ID_AST_EXPR: check(solver, return_statement_id, expected_type); break;
			default: ERROR("Missing return in scope");
		}
	}
}

char check_declaration(Solver * solver, ID node_id, ID expected_type) {
	ASSERT1(ID_IS(node_id, ID_AST_DECLARATION));

	ID decl_id = synthesis(solver, node_id);

	return ID_IS(expected_type, ID_VOID_TYPE);
}

void check_expression(Solver * solver, ID node_id, ID expected_type) {
	ASSERT1(ID_IS(node_id, ID_AST_EXPR));
	ID expr_type = synthesis(solver, node_id);
	println("expr_type: {s} vs {s}", type_to_str(expr_type), type_to_str(expected_type));
	if (is_subtype(solver, expr_type, expected_type)) {
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
