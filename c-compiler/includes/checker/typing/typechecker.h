#pragma once
#include "common/ID.h"
#include <cudd.h>
#include "common/memory/arena.h"

typedef struct group_tc {
	ID group_id;

	ID parent_group_id;
	uint32_t rank;

	ID requirement;
} Group_TC;

typedef struct requirement_tc {
	ID requirement_id;

	ID type_id;
	DdNode * world;

	ID next_requirement;
} Requirement_TC;


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

typedef struct shape_tc { // Denotes that there is a requirement of a specific named member
	ID shape_id;

	ID member_id;		// InternerID of the member that is accessed
	ID requirement_id;
} Shape_TC;

typedef struct variable_tc {
	ID variable_id;
	ID group_id;
} Variable_TC;

static inline void tc_node_init(ID id, void * node) {
	switch (id.type) {
		case ID_TC_GROUP: {
			((Group_TC *) node)->group_id = id;
			((Group_TC *) node)->parent_group_id = INVALID_ID;
			((Group_TC *) node)->rank = 0;
		} break;
		case ID_TC_REQUIREMENT: {
			((Requirement_TC *) node)->requirement_id = id;
			((Requirement_TC *) node)->type_id = INVALID_ID;
			((Requirement_TC *) node)->world = NULL;
			((Requirement_TC *) node)->next_requirement = INVALID_ID;
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
			((Variable_TC *) node)->group_id = INVALID_ID;
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
