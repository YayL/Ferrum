#include "checker/typing/groups.h"

#include "tables/registry_manager.h"
#include "checker/typing/dimensions.h"

Group_TC * solver_variable_get_root_group(Solver * solver, Variable_TC * variable) {
	ASSERT1(!ID_IS_INVALID(variable->group_id));
	Group_TC * root_group = lookup(variable->group_id);

	if (ID_IS_INVALID(root_group->parent_group_id)) {
		return root_group;
	}

	DdNode * combined_gate = variable->gate;
	Cudd_Ref(combined_gate);

	while (!ID_IS_INVALID(root_group->parent_group_id)) {
		// ASSERT1(root_group->gate != NULL);
		CUDD_AND(tmp, solver, root_group->gate, combined_gate);
		Cudd_RecursiveDeref(solver->manager, combined_gate);
		combined_gate = tmp;

		root_group = lookup(root_group->parent_group_id);
	}
	
	ASSERT1(!ID_IS_EQUAL(variable->group_id, root_group->group_id));
	CUDD_DEREF(solver, variable->gate);
	variable->group_id = root_group->group_id;
	variable->gate = combined_gate;

	return root_group;
}

void solver_add_group_requirement(Solver * solver, Group_TC * group, Requirement_TC * requirement) {
	ASSERT1(!ID_IS_INVALID(requirement->type_id));

	char matched = 0;
	DdNode * requirement_world = requirement->world;
	ID requirement_type_id = requirement->type_id;
	ID group_requirement_id = group->first_requirement;
	Requirement_TC * last_non_place = NULL;

	while (!ID_IS_INVALID(group_requirement_id)) {
		Requirement_TC * group_requirement = lookup(group_requirement_id);
		group_requirement_id = group_requirement->next_requirement;

		CUDD_AND(combined_world, solver, requirement_world, group_requirement->world);
		// CUDD_AND(combined_world, solver, combined_world_pre, solver->state);
		if (combined_world == Cudd_ReadLogicZero(solver->manager)) {
			// println("Invalid combination: {s} AND {s}", type_to_str(requirement->type_id), type_to_str(group_requirement->type_id));
			// print_possibilities(solver, requirement_world);
			// puts("");
			// print_possibilities(solver, group_requirement->world);
			// puts("");
			continue;
		}

		if (solver_decompose(solver, group_requirement->type_id, requirement_type_id, combined_world)) {
			// Add requirement's world as an option in group_requirement (group_requirement->world OR requirement->world)
			matched = 1;
			if (requirement_world == NULL) {
				continue;
			}

			CUDD_OR(with_world_alternative, solver, requirement_world, group_requirement->world);
			Cudd_RecursiveDeref(solver->manager, group_requirement->world);
			ASSERT1(with_world_alternative != NULL);

			group_requirement->world = with_world_alternative;
		} else {
			// println("{s} != {s}", type_to_str(requirement->type_id), type_to_str(group_requirement->type_id));
			// print_possibilities(solver, requirement_world);
			// puts("");
			// print_possibilities(solver, group_requirement->world);
			// puts("");
			SOLVER_ADD_INVALID(solver, combined_world);
		}
	}
	
	if (!matched) {
		// Add requirement to front of group requirements
		requirement->next_requirement = group->first_requirement;
		group->first_requirement = requirement->requirement_id;
	}
}

void solver_add_variable_group_requirement(Solver * solver, ID variable_id, ID requirement_type_id, DdNode * world) {
	ASSERT1(ID_IS(variable_id, ID_TC_VARIABLE));
	ASSERT1(!ID_IS(requirement_type_id, ID_TC_VARIABLE));

	Variable_TC * variable = lookup(variable_id);
	CUDD_OR(tmp, solver, world, variable->gate);
	CUDD_DEREF(solver, variable->gate);
	variable->gate = tmp;

	Group_TC * group;
	if (ID_IS_INVALID(variable->group_id)) {
		group = tc_allocate(ID_TC_GROUP);
		variable->group_id = group->group_id;
	} else {
		group = solver_variable_get_root_group(solver, variable);
	}

	Requirement_TC * requirement = tc_allocate(ID_TC_REQUIREMENT);
	requirement->type_id = requirement_type_id;
	requirement->world = world;

	solver_add_group_requirement(solver, group, requirement);
}

void group_print_requirements(Solver * solver, ID group_id) {
	ASSERT1(ID_IS(group_id, ID_TC_GROUP));

	Group_TC group = LOOKUP(group_id, Group_TC);

	ID requirement_id = group.first_requirement;
	while (!ID_IS_INVALID(requirement_id)) {
		Requirement_TC requirement = LOOKUP(requirement_id, Requirement_TC);

		print("{s}: ", type_to_str(requirement.type_id));
		print_possibilities(solver, requirement.world);

		requirement_id = requirement.next_requirement;
	}
}

void solver_unify_vars_groups(Solver * solver, ID id1, ID id2, DdNode * world) {
	ASSERT1(ID_IS(id1, ID_TC_VARIABLE));
	ASSERT1(ID_IS(id2, ID_TC_VARIABLE));
	Variable_TC * const var1 = lookup(id1);
	Variable_TC * const var2 = lookup(id2);
	

	Group_TC * group1;
	Group_TC * group2;
	
	if (ID_IS_INVALID(var1->group_id) && ID_IS_INVALID(var2->group_id)) {
		group1 = group2 = tc_allocate(ID_TC_GROUP);
		var1->group_id = var2->group_id = group1->group_id;
	} else if (ID_IS_INVALID(var1->group_id)) {
		group1 = group2 = solver_variable_get_root_group(solver, var2);
		var1->group_id = group1->group_id;
	} else if (ID_IS_INVALID(var2->group_id)) {
		group1 = group2 = solver_variable_get_root_group(solver, var1);
		var2->group_id = group2->group_id;
	} else {
		group1 = solver_variable_get_root_group(solver, var1);
		group2 = solver_variable_get_root_group(solver, var2);
	}

	// print("World: ");
	// print_possibilities(solver, world);
	// print("Var {u} (Group {u}): ", var1->variable_id.id, var1->group_id.id);
	// print_possibilities(solver, var1->gate);
	// print("Var {u} (Group {u}): ", var2->variable_id.id, var2->group_id.id);
	// print_possibilities(solver, var2->gate);

	/*
		var1:  NULL
		var2:  D1[0]
		world: D1[1]

		path_via_1: D1[1]
		path_via_2: D1[0] AND D1[1]

		new_gate1: 0 (NULL OR (D1[0] AND D1[1]))
		new_gate2: D1[0|1] (D1[0] OR (NULL AND D1[1]))
	*/

	CUDD_AND(path_via_1, solver, var1->gate, world);
	CUDD_AND(path_via_2, solver, var2->gate, world);
	CUDD_OR(new_gate1, solver, var1->gate, path_via_2);
	CUDD_OR(new_gate2, solver, var2->gate, path_via_1);
	CUDD_DEREF(solver, var1->gate);
	CUDD_DEREF(solver, var2->gate);
	CUDD_DEREF(solver, path_via_1);
	CUDD_DEREF(solver, path_via_2);
	var1->gate = new_gate1;
	var2->gate = new_gate2;

	// CUDD_OR(tmp1, solver, world, var1->gate);
	// CUDD_OR(tmp2, solver, world, var2->gate);
	// CUDD_DEREF(solver, var1->gate);
	// CUDD_DEREF(solver, var2->gate);
	// var1->gate = tmp1;
	// var2->gate = tmp2;

	if (ID_IS_EQUAL(group1->group_id, group2->group_id)) {
		return;
	}

	ASSERT1(ID_IS_INVALID(group1->parent_group_id));
	ASSERT1(ID_IS_INVALID(group2->parent_group_id));

	Group_TC * parent, * child;
	if (group1->rank < group2->rank) {
		parent = group2, child = group1;
	} else {
		parent = group1, child= group2;
	}

	child->parent_group_id = parent->group_id;
	child->rank = parent->rank + 1; // this shouldn't ever have to be used but good to add anyway

	if (ID_IS_INVALID(child->first_requirement)) {
		return;
	}

	if (ID_IS_INVALID(parent->first_requirement)) {
		parent->first_requirement = child->first_requirement;
		return;
	}

	ID parent_start_requirment_id = parent->first_requirement;
	ID next_child_requirement_id = child->first_requirement;

	Requirement_TC * extracted = NULL;
	ID first_extracted = INVALID_ID;
	while (!ID_IS_INVALID(next_child_requirement_id)) {
		Requirement_TC * child_requirement = lookup(next_child_requirement_id);
		next_child_requirement_id = child_requirement->next_requirement;
		child_requirement->next_requirement = INVALID_ID;

		CUDD_AND(tmp, solver, child_requirement->world, world);
		CUDD_DEREF(solver, child_requirement->world);
		child_requirement->world = tmp;

		solver_add_group_requirement(solver, parent, child_requirement);
		
		// Didn't find a match and added child_requirement to front of parent's requirement list
		if (ID_IS_EQUAL(parent->first_requirement, child_requirement->requirement_id)) {
			// Move parent requirement back to before child was added
			parent->first_requirement = child_requirement->next_requirement;

			if (extracted == NULL) {
				first_extracted = child_requirement->requirement_id;
				extracted = child_requirement;
			} else {
				// Add child requirement as next in extracted
				extracted->next_requirement = child_requirement->requirement_id;
				extracted = child_requirement;
			}

			// Detach extracted from parent
			extracted->next_requirement = INVALID_ID;
		}
	}

	if (extracted != NULL) {
		extracted->next_requirement = parent->first_requirement;
		parent->first_requirement = first_extracted;
	}

	child->first_requirement = INVALID_ID;
}
