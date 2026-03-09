#pragma once

#include "common/ID.h"
#include "common/memory/arena.h"
#include <cudd.h>
#include "checker/typing/solver.h"

struct template_variable {
	ID name_id;
	ID variable_id;
};

ID replace_templates_in_type_with_template_variables(Solver * solver, ID type_id, const Arena templates);
void populate_template_list_from_arena(Solver * solver, Arena arena, Arena * templates);

ID find_template(const Arena templates, ID name_id);
