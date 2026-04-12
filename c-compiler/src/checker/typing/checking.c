#include "checker/typing/checking.h"

#include "checker/typing/synthesis.h"
#include "checker/typing/subtyping.h"

static inline char check_scope(Solver * solver, ID node_id, ID expected_type) {
	ASSERT1(ID_IS(node_id, ID_AST_SCOPE));
	return is_subtype(solver, synthesis(solver, node_id), expected_type, 0);
}

static inline char check_declaration(Solver * solver, ID node_id, ID expected_type) {
	ASSERT1(ID_IS(node_id, ID_AST_DECLARATION));

	synthesis(solver, node_id);

	return ID_IS(expected_type, ID_VOID_TYPE);
}

static inline char check_expression(Solver * solver, ID node_id, ID expected_type) {
	ASSERT1(ID_IS(node_id, ID_AST_EXPR));
	return is_subtype(solver, synthesis(solver, node_id), expected_type, 0);
}

char check(Solver * solver, ID node_id, ID expected_type) {
	switch (node_id.type) {
		case ID_TUPLE_TYPE: return is_subtype(solver, node_id, expected_type, 0);
		case ID_AST_SCOPE: return check_scope(solver, node_id, expected_type);
		case ID_AST_DECLARATION: return check_declaration(solver, node_id, expected_type);
		case ID_AST_EXPR: return check_expression(solver, node_id, expected_type);
		// case ID_AST_IF: check_if(solver, node_id, expected_type); break;
		default:
			FATAL("Unimplemented AST type \"{s}\"", id_type_to_string(node_id.type));
	}
}
