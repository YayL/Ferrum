#pragma once

#include "common/ID.h"
#include "common/memory/arena.h"
#include <cudd.h>

typedef struct constraint_tc {
	ID constraint_id;

	ID from;
	ID to;
	DdNode * choice;

	// Solver info
	ID prev_lower_bound_for_to; // Next constraint where 'to' is the same
	ID prev_upper_bound_for_from; // Next constraint where 'from' is the same
} Constraint_TC;

typedef struct dimension_tc {
	ID dimension_id;

	uint32_t first_bit_index;
	uint32_t bit_count;

	Arena candidates;
} Dimension_TC;

typedef struct cast_tc {
	ID cast_id;

	ID variable_id;
	ID dimension_id;
} Cast_TC;

typedef struct generic_tc {
	ID generic_id;

	ID symbol_id;
	ID args_id;
	ID constraint_id;
} Generic_TC;

typedef struct shape_tc { // Denotes that there is a requirement of a specific member
	ID shape_id;

	ID member_id;		// InternerID of the member that is accessed
	ID requirement_id;
} Shape_TC;

typedef struct variable_tc {
	ID variable_id;

	// Solver info
	ID lower_bound;
	ID upper_bound;
} Variable_TC;

static inline void tc_node_init(ID id, void * node) {
	switch (id.type) {
		case ID_TC_CONSTRAINT: {
			((Constraint_TC *) node)->constraint_id = id;
			((Constraint_TC *) node)->from = INVALID_ID;
			((Constraint_TC *) node)->to = INVALID_ID;
			((Constraint_TC *) node)->choice = NULL;
			((Constraint_TC *) node)->prev_lower_bound_for_to = INVALID_ID;
			((Constraint_TC *) node)->prev_upper_bound_for_from = INVALID_ID;
		} break;
		case ID_TC_GENERIC: {
			((Generic_TC *) node)->generic_id = id;
			((Generic_TC *) node)->symbol_id = INVALID_ID;
			((Generic_TC *) node)->args_id = INVALID_ID;
			((Generic_TC *) node)->constraint_id = INVALID_ID;
		} break;
		case ID_TC_DIMENSION: {
			((Dimension_TC *) node)->dimension_id = id;
			((Dimension_TC *) node)->candidates = arena_init(sizeof(ID));
		} break;
		case ID_TC_SHAPE: {
			((Shape_TC *) node)->shape_id = id;
			((Shape_TC *) node)->member_id = INVALID_ID;
			((Shape_TC *) node)->requirement_id = INVALID_ID;
		} break;
		case ID_TC_VARIABLE: {
			((Variable_TC *) node)->variable_id = id;
			((Variable_TC *) node)->lower_bound = INVALID_ID;
			((Variable_TC *) node)->upper_bound = INVALID_ID;
		} break;
		case ID_TC_CAST: {
			((Cast_TC *) node)->cast_id = id;
			((Cast_TC *) node)->dimension_id = INVALID_ID;
			((Cast_TC *) node)->variable_id = INVALID_ID;
		} break;
		default:
			FATAL("Invalid ID type: {s}", id_type_to_string(id.type));
	}
}

struct template_variable {
	ID name_id;
	ID variable_id;
};

void generate_template_constraints(ID node_id, Arena * templates);
ID replace_templates_in_type_with_template_variables(ID type_id, const Arena templates, char allow_cast);
