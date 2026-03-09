#include "checker/typing/groups.h"

#include "tables/registry_manager.h"

Group_TC * group_find_root_group(ID group_id) {
	Group_TC * group;

	do {
		ASSERT1(ID_IS(group_id, ID_TC_GROUP));
		group = lookup(group_id);
		group_id = group->parent_group_id;
	} while (!ID_IS_INVALID(group_id));
	
	return group;
}

void solver_add_group_requirement(struct solver * solver, Group_TC * group, Requirement_TC * requirement, DdNode * world) {
	ASSERT1(!ID_IS_INVALID(requirement->type_id));

	char matched = 0;
	DdNode * requirement_world = requirement->world;
	ID requirement_type_id = requirement->type_id;
	ID group_requirement_id = group->requirement;
	while (!ID_IS_INVALID(group_requirement_id)) {
		Requirement_TC * group_requirement = lookup(group_requirement_id);
		DdNode * combined_world = cudd_both(solver, requirement_world, group_requirement->world);

		if (solver_decompose(solver, group_requirement->type_id, requirement_type_id, combined_world)) {
			// Add requirement's world as an option in group_requirement (group_requirement->world OR requirement->world)
			matched = 1;
			if (requirement_world == NULL) {
				break;
			}

			DdNode * combined_world = cudd_either(solver, requirement_world, group_requirement->world);
			Cudd_RecursiveDeref(solver->resolver.manager, group_requirement->world);
			ASSERT1(combined_world != NULL);

			group_requirement->world = combined_world;
			break;
		} // else check next

		group_requirement_id = group_requirement->next_requirement;
	}
	
	if (!matched) {
		// Add requirement to front of group requirements
		requirement->next_requirement = group->requirement;
		group->requirement = requirement->requirement_id;

		// if (requirement
	}
}

void solver_add_variable_group_requirement(struct solver * ctx, ID variable_id, ID requirement_type_id, DdNode * world) {
	ASSERT1(ID_IS(variable_id, ID_TC_VARIABLE));
	ASSERT1(!ID_IS(requirement_type_id, ID_TC_VARIABLE));

	Variable_TC * variable = lookup(variable_id);
	Group_TC * group;
	
	if (ID_IS_INVALID(variable->group_id)) {
		group = tc_allocate(ID_TC_GROUP);
		variable->group_id = group->group_id;
	} else {
		group = group_find_root_group(variable->group_id);
	}

	Requirement_TC * requirement = tc_allocate(ID_TC_REQUIREMENT);
	requirement->type_id = requirement_type_id;
	requirement->world = world;

	solver_add_group_requirement(ctx, group, requirement, NULL);
}
