#pragma once

#include "codegen/AST.h"

void add_function_to_module(struct AST * module, struct AST * function);
void include_module(struct AST * dest, struct AST * src);
struct AST * find_module(struct AST * root, const char * module_name);

