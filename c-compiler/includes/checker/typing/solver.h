#pragma once

#include "checker/typing/gathering.h"
#include "checker/typing/dimensions.h"
#include "common/data/deque.h"

IMPLEMENT_DEQUE(ID)

struct solver {
	DEQUE_T(ID) worklist;
	Arena err_constraints;
	Dim_Resolver resolver;
};

void solver_initialize(struct solver * ctx);
void solver_process_worklist(struct solver * ctx);
