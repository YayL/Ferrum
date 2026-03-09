#pragma once

#include "common/data/deque.h"
#include "parser/types.h"

IMPLEMENT_DEQUE(ID);

#define TYPE_IDS_TO_CONSTRAINT_PAIR(FROM, TO) ((struct constraint_type_id_pair) {.from = (FROM), .to = (TO)})

struct constraint_type_id_pair {
	ID from, to;
};

static inline char constraint_type_id_pair_eq(struct constraint_type_id_pair pair1, struct constraint_type_id_pair pair2) {
	return type_check_deep_equal(pair1.from, pair2.from) && type_check_deep_equal(pair1.to, pair2.to);
}

static inline uint64_t constraint_type_id_hash(struct constraint_type_id_pair pair) {
	return type_id_to_hash(pair.from) | type_id_to_hash(pair.to);
}

KHASH_INIT(map_type_id_pair_to_constraint, struct constraint_type_id_pair, ID, 1, constraint_type_id_hash, constraint_type_id_pair_eq);

#include "checker/typing/dimensions.h"
struct solver {
	DEQUE_T(ID) worklist;
	khash_t(map_type_id_pair_to_constraint) constraints;
	Arena err_constraints;
	Dim_Resolver resolver;
};

void solver_initialize(struct solver * ctx);
void solver_process_worklist(struct solver * ctx);

void solver_add_new_flow(struct solver * ctx, ID from, ID to, DdNode * from_choice, DdNode * to_choice);
