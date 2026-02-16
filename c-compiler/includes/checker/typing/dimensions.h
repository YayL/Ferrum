#pragma once

#include "common/ID.h"
#include <cudd.h>
#include "checker/typing/gathering.h"

#include "tables/registry_manager.h"
#include "math.h"

typedef struct dimension_resolution {
	DdManager * manager;
	DdNode * state;
	uint32_t bit_variable_count;
} Dim_Resolver;

void resolver_print_possibilities(Dim_Resolver resolver);
void resolver_add_invalid_choice(Dim_Resolver * resolver, DdNode * choice);
DdNode * dimension_get_choice(Dim_Resolver resolver, ID dimension_id, size_t choice);

static uint32_t dimension_init_choices(Dim_Resolver resolver, Dimension_TC * dimension) { 
	ASSERT1(dimension->candidates.size > 0);

	dimension->first_bit_index = resolver.bit_variable_count;
	dimension->bit_count = dimension->candidates.size == 1 
						? 1
						: ceil(log2((double) dimension->candidates.size));

	return dimension->bit_count;
}


static inline Dim_Resolver dimension_resolver_init() {
	DdManager * manager = Cudd_Init(0, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0);
	Dim_Resolver resolver = { .manager = manager, .bit_variable_count = 0, .state = NULL };

	DdNode * state = Cudd_ReadOne(manager);
	Dimension_TC * dimension;

	LOOP_OVER_REGISTRY(Dimension_TC, dimension, {
			resolver.bit_variable_count += dimension_init_choices(resolver, dimension);
	
			DdNode * dim_group = dimension_get_choice(resolver, dimension->dimension_id, 0);
	
			// start at 1, since we already retrieved 0th
			for (size_t i = 1; i < dimension->candidates.size; ++i) {
				DdNode * encoded_choice = dimension_get_choice(resolver, dimension->dimension_id, i);
				DdNode * tmp = Cudd_bddOr(resolver.manager, encoded_choice, dim_group);
				Cudd_Ref(tmp);
				Cudd_RecursiveDeref(resolver.manager, encoded_choice);
				Cudd_RecursiveDeref(resolver.manager, dim_group);
				dim_group = tmp;
			}
	
			DdNode * tmp = Cudd_bddAnd(resolver.manager, dim_group, state);
			Cudd_Ref(tmp);
			Cudd_RecursiveDeref(resolver.manager, dim_group);
			Cudd_RecursiveDeref(resolver.manager, state);
			state = tmp;
	});

	// resolver.state = state;
	// resolver_print_possibilities(resolver);
	//
	// DdNode * choice_1 = dimension_get_choice(resolver, (ID) {.id = 3, .type = ID_TC_DIMENSION}, 0);
	// DdNode * choice_2 = dimension_get_choice(resolver, (ID) {.id = 3, .type = ID_TC_DIMENSION}, 2);
	// DdNode * choice_3 = dimension_get_choice(resolver, (ID) {.id = 1, .type = ID_TC_DIMENSION}, 2);
	//
	// state = Cudd_bddAnd(resolver.manager, Cudd_Not(choice_1), state);
	// state = Cudd_bddAnd(resolver.manager, Cudd_Not(choice_2), state);
	// state = Cudd_bddAnd(resolver.manager, Cudd_Not(choice_3), state);
	//
	// resolver.state = state;
	// resolver_print_possibilities(resolver);

	resolver.state = state;
	return resolver;
}
