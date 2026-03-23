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
char solver_unify(Solver * solver, ID id1, ID id2, DdNode * world);
void solver_add_variable_group_requirement(Solver * ctx, ID id1, ID id2, DdNode * world);

void solver_generate_where_constraints(Solver * solver, Arena where_arena, Arena templates, DdNode * parent_choice);
void solver_generate_template_constraints(Solver * solver, ID node_id, Arena * templates, DdNode * parent_choice);

#define CUDD_DEREF(SOLVER, NODE) if ((NODE) != NULL) { Cudd_RecursiveDeref((SOLVER)->manager, NODE); }
#define CUDD_FUNC(FUNC, VAR, SOLVER, NODE1, NODE2) \
	DdNode * VAR; \
	if ((NODE1) == NULL) { \
		(VAR) = (NODE2); \
	} else if ((NODE2) == NULL) { \
		(VAR) = (NODE1); \
	} else { \
		VAR = Cudd_bdd##FUNC((SOLVER)->manager, (NODE1), (NODE2)); \
	} \
	ASSERT1((VAR) != NULL); \
	Cudd_Ref(VAR);

#define CUDD_AND(VAR, SOLVER, NODE1, NODE2) CUDD_FUNC(And, VAR, SOLVER, NODE1, NODE2)
#define CUDD_OR(VAR, SOLVER, NODE1, NODE2) CUDD_FUNC(Or, VAR, SOLVER, NODE1, NODE2)

#define SOLVER_AND(SOLVER, NODE) { \
	CUDD_AND(tmp, SOLVER, NODE, (SOLVER)->state); \
	Cudd_RecursiveDeref((SOLVER)->manager, (SOLVER)->state); \
	(SOLVER)->state = tmp; \
}
// if (tmp == Cudd_ReadLogicZero((SOLVER)->manager)) { print_possibilities(SOLVER, (SOLVER)->state); println("AND"); print_possibilities(SOLVER, NODE); }

#define SOLVER_OR(SOLVER, NODE) { \
	CUDD_OR(tmp, SOLVER, NODE, (SOLVER)->state); \
	Cudd_RecursiveDeref((SOLVER)->manager, (SOLVER)->state); \
	(SOLVER)->state = tmp; \
}
// if (tmp == Cudd_ReadLogicZero((SOLVER)->manager)) { print_possibilities(SOLVER, (SOLVER)->state); println("OR"); print_possibilities(SOLVER, NODE); }

#define SOLVER_ADD_INVALID(SOLVER, INVALIDATE) SOLVER_AND(SOLVER, Cudd_Not(INVALIDATE))

