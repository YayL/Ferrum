#pragma once

#include "codegen/AST.h"
#include "codegen/gen.h"
#include "codegen/llvm.h"

const char * gen_builtin(struct Ast * ast, struct Ast * self_type);
