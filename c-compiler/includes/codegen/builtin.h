#pragma once

#include "codegen/AST.h"
#include "codegen/gen.h"
#include "codegen/llvm.h"

const char * gen_builtin(struct AST * ast, struct AST * self_type);
