#pragma once

#include "common/ID.h"
#include "common/memory/arena.h"

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

typedef struct obligation {
	struct tc_context_info info;
	ID type_id;
	ID offender_id;

	enum obligation_type {
		IMPLEMENTATION_OBLIGATION, // Known candidate but unresolved implementation check due to existential type
		MEMBER_OBLIGATION, // Member access on existential type
		FUNCTION_OBLIGATION // Multiple function candidates with existential in arguments
	} obligation_type;
} Obligation;

#define MARKER_COUNT_GET(TYPE) TYPE##_count
#define MARKER_COUNT_GEN(ENUM, _, TYPE, ...) uint32_t MARKER_COUNT_GET(TYPE);
typedef struct marker {
	struct tc_context_info info;
	TYPE_CHECKING_CONTEXT_KINDS(MARKER_COUNT_GEN);
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

void solver_type_init(enum id_type type, void * ref);

void solver_collect_templates(Solver * solver, ID node_id);
ID solver_find_symbol_term_var(Solver * solver, ID symbol_id);
ID solver_get_template_type(ID name_id);
ID solver_replace_templates(ID type_id);
ID solver_deflate_type(ID id);
char solver_implementation_is_valid(Solver * solver, ID implementation_id, const Arena templates);
char solver_validate_trait_implementation(Solver * solver, ID id);
char solver_validate_where_clauses(Solver * solver, ID node_id);

void solver_reset_to_marker(Solver * solver, ID marker_id);
