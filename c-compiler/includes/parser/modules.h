#pragma once

#include "codegen/AST.h"

void add_function_to_module(struct Ast * module, struct Ast * function);
void include_module(struct Ast * dest, struct Ast * src);
struct Ast * find_module(struct Ast * root, const char * module_name);

