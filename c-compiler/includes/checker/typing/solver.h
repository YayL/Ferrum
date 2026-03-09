#pragma once

#include "checker/typing/dimensions.h"

typedef struct solver {
	// Arena err_constraints;
	Dim_Resolver resolver;
} Solver;


void solver_initialize(Solver * ctx);

char solver_decompose(Solver * ctx, ID id1, ID id2, DdNode * world);
void solver_unify(Solver * solver, ID id1, ID id2, DdNode * world);
void solver_add_variable_group_requirement(Solver * ctx, ID id1, ID id2, DdNode * world);

void generate_template_constraints(Solver * solver, ID node_id, Arena * templates, DdNode * parent_choice);

static inline DdNode * cudd_both(struct solver * solver, DdNode * node1, DdNode * node2) {
	if (node1 == NULL) {
		return node2;
	} else if (node2 == NULL) {
		return node1;
	}
	
	DdNode * new_node = Cudd_bddAnd(solver->resolver.manager, node1, node2);
	Cudd_Ref(new_node);

	return new_node;
}

static inline DdNode * cudd_either(struct solver * solver, DdNode * node1, DdNode * node2) {
	if (node1 == NULL) {
		return node2;
	} else if (node2 == NULL) {
		return node1;
	}
	
	DdNode * new_node = Cudd_bddOr(solver->resolver.manager, node1, node2);
	Cudd_Ref(new_node);

	return new_node;
}

static inline void solver_add_invalid_world(Solver * solver, DdNode * world) {
	DdNode * new_state = cudd_both(solver, Cudd_Not(world), solver->resolver.state);
	Cudd_RecursiveDeref(solver->resolver.manager, solver->resolver.state);
	ASSERT1(new_state != NULL);

	solver->resolver.state = new_state;
}
