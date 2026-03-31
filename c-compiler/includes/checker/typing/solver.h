#pragma once

#include "common/ID.h"

struct tc_context_info {
	ID id;
	uint32_t context_order_id;
};

// Variable with known type
typedef struct termvar {
	struct tc_context_info info;
	ID symbol_id;
	ID type;
} TermVar;

typedef struct template {
	struct tc_context_info info;
	ID name_id;
	ID type_id;
} Template;

// Generic type
typedef struct typevar {
	struct tc_context_info info;
} TypeVar;

// Type to solve (not known at first)
typedef struct existenial {
	struct tc_context_info info;
	ID solved_type;
} Existential;

typedef struct marker {
	struct tc_context_info info;
} Marker;

typedef struct solver {
	ID id;

	uint32_t context_element_count;
} Solver;

Solver solver_initialize();

void * tc_allocate(enum id_type type);
static inline void * solver_allocate(Solver * solver, enum id_type type) { 
	void * ref = tc_allocate(type); 
	((struct tc_context_info *) ref)->context_order_id = ++solver->context_element_count;
	return ref;
}

void solver_collect_templates(Solver * solver, ID node_id);
ID solver_get_template_type(ID name_id);
ID solver_replace_templates(ID type_id);
