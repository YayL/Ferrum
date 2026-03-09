#pragma once

#include <cudd.h>
#include "common/ID.h"
#include "common/memory/arena.h"

typedef struct solver {
	DdManager * manager;
	DdNode * state;
	uint32_t bit_variable_count;
} Solver;

void solver_initialize(Solver * ctx);

char solver_decompose(Solver * ctx, ID id1, ID id2, DdNode * world);
void solver_unify(Solver * solver, ID id1, ID id2, DdNode * world);
void solver_add_variable_group_requirement(Solver * ctx, ID id1, ID id2, DdNode * world);

void solver_generate_where_constraints(Solver * solver, Arena where_arena, Arena templates, DdNode * parent_choice);
void solver_generate_template_constraints(Solver * solver, ID node_id, Arena * templates, DdNode * parent_choice);

static inline DdNode * cudd_both(Solver * solver, DdNode * node1, DdNode * node2) {
	if (node1 == NULL) {
		Cudd_Ref(node2);
		return node2;
	} else if (node2 == NULL) {
		Cudd_Ref(node1);
		return node1;
	}
	
	DdNode * new_node = Cudd_bddAnd(solver->manager, node1, node2);
	Cudd_Ref(new_node);

	return new_node;
}

static inline DdNode * cudd_either(Solver * solver, DdNode * node1, DdNode * node2) {
	if (node1 == NULL) {
		return node2;
	} else if (node2 == NULL) {
		return node1;
	}
	
	DdNode * new_node = Cudd_bddOr(solver->manager, node1, node2);
	Cudd_Ref(new_node);

	return new_node;
}

static inline DdNode * solver_both_and_global(Solver * solver, DdNode * node1, DdNode * node2) {
	return cudd_both(solver, cudd_both(solver, node1, node2), solver->state);
}

static inline void solver_add_invalid_world(Solver * solver, DdNode * world) {
	DdNode * new_state = cudd_both(solver, Cudd_Not(world), solver->state);
	Cudd_RecursiveDeref(solver->manager, solver->state);
	ASSERT1(new_state != NULL);

	solver->state = new_state;
}
