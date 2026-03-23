#pragma once

#include "common/ID.h"
#include "math.h"
#include "checker/typing/typechecker.h"
#include "checker/typing/solver.h"

void print_possibilities(Solver * solver, DdNode * state);
void resolver_add_invalid_choice(Solver * solver, DdNode * choice);
DdNode * dimension_get_choice(Solver * solver, ID dimension_id, size_t choice, DdNode * world);

DdNode * get_clean_dimension_state(Solver * solver, DdNode * state);

static void dimension_init_choices(Solver * solver, Dimension_TC * dimension, DdNode * parent_choice) { 
	ASSERT1(dimension->bit_count == 0);
	ASSERT1(dimension->candidates.size > 0);

	dimension->first_bit_index = solver->bit_variable_count;
	dimension->bit_count = dimension->candidates.size == 1 
						? 1
						: ceil(log2((double) dimension->candidates.size));

	DdNode * dim_group = dimension_get_choice(solver, dimension->dimension_id, 0, NULL);

	// start at 1, since we already retrieved 0th
	for (size_t i = 1; i < dimension->candidates.size; ++i) {
		DdNode * encoded_choice = dimension_get_choice(solver, dimension->dimension_id, i, NULL);
		CUDD_OR(tmp, solver, encoded_choice, dim_group);
		Cudd_RecursiveDeref(solver->manager, encoded_choice);
		Cudd_RecursiveDeref(solver->manager, dim_group);
		dim_group = tmp;
	}

	if (parent_choice != NULL) {
		// parent_choice -> dim_group (parent implies dim_group)
		CUDD_OR(tmp, solver, Cudd_Not(parent_choice), dim_group);
		Cudd_RecursiveDeref(solver->manager, dim_group);
		dim_group = tmp;
	} // else since parent_choice == NULL, that is equivalent to: 0 OR dim_group -> dim_group (i.e we don't need to change anything)

	if (solver->state != NULL) {
		SOLVER_AND(solver, dim_group);
		Cudd_RecursiveDeref(solver->manager, dim_group);
	} else {
		solver->state = dim_group;
	}

	solver->bit_variable_count += dimension->bit_count;
}

