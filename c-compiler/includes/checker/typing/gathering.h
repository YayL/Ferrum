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
	ID next_incoming_for_to; // Next constraint where 'to' is the same
	ID next_outgoing_for_from; // Next constraint where 'from' is the same
} Constraint_TC;

typedef struct dimension_tc {
	ID dimension_id;

	Arena bit_variables;
	Arena candidates;
} Dimension_TC;

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
	ID incoming_head;
	ID outgoing_head;
} Variable_TC;

static inline void tc_node_init(ID id, void * node) {
	switch (id.type) {
		case ID_TC_CONSTRAINT: {
			((Constraint_TC *) node)->constraint_id = id;
			((Constraint_TC *) node)->from = INVALID_ID;
			((Constraint_TC *) node)->to = INVALID_ID;
			((Constraint_TC *) node)->next_incoming_for_to = INVALID_ID;
			((Constraint_TC *) node)->next_outgoing_for_from = INVALID_ID;
		} break;
		case ID_TC_GENERIC: {
			((Generic_TC *) node)->generic_id = id;
			((Generic_TC *) node)->symbol_id = INVALID_ID;
			((Generic_TC *) node)->args_id = INVALID_ID;
			((Generic_TC *) node)->constraint_id = INVALID_ID;
		} break;
		case ID_TC_DIMENSION: {
			((Dimension_TC *) node)->dimension_id = id;
		} break;
		case ID_TC_SHAPE: {
			((Shape_TC *) node)->shape_id = id;
			((Shape_TC *) node)->member_id = INVALID_ID;
			((Shape_TC *) node)->requirement_id = INVALID_ID;
		} break;
		case ID_TC_VARIABLE: {
			((Variable_TC *) node)->variable_id = id;
			((Variable_TC *) node)->incoming_head = INVALID_ID;
			((Variable_TC *) node)->outgoing_head = INVALID_ID;
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
ID replace_templates_in_type_with_template_variables(ID type_id, const Arena templates);
