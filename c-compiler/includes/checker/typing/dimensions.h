#pragma once

#include "common/ID.h"
#include <cudd.h>
#include "checker/typing/gathering.h"

typedef struct dimension_resolution {
	DdManager * manager;
} Dim_Resolver;

static inline Dim_Resolver dimension_resolver_init() {
	return (Dim_Resolver) {
		.manager = Cudd_Init(0, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0)
	};
}

void dimension_init_choices(Dim_Resolver resolver, Dimension_TC * dimension);
void resolver_encode_choice(Dim_Resolver resolver, ID dimension, size_t choice);
