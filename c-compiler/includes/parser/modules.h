#pragma once

#include "codegen/AST.h"
#include "parser/parser.h"

struct AST * add_module(struct Parser * parser, char * path);
struct AST * find_module(struct AST * root, const char * module_path);
void add_import_to_module(struct AST * module, struct AST * imported_module, unsigned int name_id);
