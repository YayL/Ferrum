#pragma once

#include "parser/AST.h"
#include "parser/parser.h"

ID add_module(struct Parser * parser, char * path);
ID find_module(a_root * root, const char * module_path);
