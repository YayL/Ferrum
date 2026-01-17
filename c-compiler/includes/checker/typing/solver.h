#pragma once

#include "checker/typing/gathering.h"
#include "common/data/deque.h"

IMPLEMENT_DEQUE(ID)

struct solver {
	DEQUE_T(ID) worklist;
};

void solver_initialize(struct solver * ctx);
void solver_process_worklist(struct solver * ctx);
