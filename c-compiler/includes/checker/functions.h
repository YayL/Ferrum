#pragma once

#include "parser/operators.h"
#include "parser/AST.h"

void resolve_function_from_call(ID node_id, Arena extra_templates);
void resolve_function_from_operator(ID node_id);

ID get_member_function(const char * member_name, ID arguments_type, ID return_type, ID scope_id);
ID get_function_for_operator(struct Operator op, ID left, ID right, ID scope_id);

char is_declared_function(ID name_id, ID scope_id);
