#pragma once

#include "common/ID.h"
#include <cudd.h>
#include "math.h"

typedef struct dimension_resolution {
	DdManager * manager;
	DdNode * state;
	uint32_t bit_variable_count;
} Dim_Resolver;

#include "checker/typing/gathering.h"

Dim_Resolver dimension_resolver_init();
void print_possibilities(Dim_Resolver resolver, DdNode * state);
void resolver_add_invalid_choice(Dim_Resolver * resolver, DdNode * choice);
DdNode * dimension_get_choice(Dim_Resolver resolver, ID dimension_id, size_t choice);

static void dimension_init_choices(Dim_Resolver * resolver, Dimension_TC * dimension, DdNode * parent_choice) { 
	ASSERT1(dimension->bit_count == 0);
	ASSERT1(dimension->candidates.size > 0);

	dimension->first_bit_index = resolver->bit_variable_count;
	dimension->bit_count = dimension->candidates.size == 1 
						? 1
						: ceil(log2((double) dimension->candidates.size));

	DdNode * dim_group = dimension_get_choice(*resolver, dimension->dimension_id, 0);

	// start at 1, since we already retrieved 0th
	for (size_t i = 1; i < dimension->candidates.size; ++i) {
		DdNode * encoded_choice = dimension_get_choice(*resolver, dimension->dimension_id, i);
		DdNode * tmp = Cudd_bddOr(resolver->manager, encoded_choice, dim_group);
		Cudd_Ref(tmp);
		Cudd_RecursiveDeref(resolver->manager, encoded_choice);
		Cudd_RecursiveDeref(resolver->manager, dim_group);
		dim_group = tmp;
	}

	if (parent_choice != NULL) {
		// parent_choice -> dim_group
		DdNode * tmp = Cudd_bddOr(resolver->manager, Cudd_Not(parent_choice), dim_group);
		Cudd_Ref(tmp);
		Cudd_RecursiveDeref(resolver->manager, dim_group);
		dim_group = tmp;
	}

	if (resolver->state != NULL) {
		DdNode * tmp = Cudd_bddAnd(resolver->manager, dim_group, resolver->state);
		Cudd_Ref(tmp);
		Cudd_RecursiveDeref(resolver->manager, dim_group);
		Cudd_RecursiveDeref(resolver->manager, resolver->state);
		resolver->state = tmp;
	} else {
		resolver->state = dim_group;
	}

	resolver->bit_variable_count += dimension->bit_count;
}

