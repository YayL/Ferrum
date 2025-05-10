#pragma once

#include "common/common.h"
#include "parser/operators.h"

struct Ast * get_member_function(const char * member_name, struct Ast * marker, struct List * arguments_type, struct Ast * scope);
struct Ast * get_function_for_operator(struct Operator * op, struct Ast * left, struct Ast * right, struct Ast ** self_type, struct Ast * scope);
struct Ast * get_declared_function(const char * name, struct List * list1, struct Ast * scope);

char is_declared_function(char * name, struct Ast * scope);
