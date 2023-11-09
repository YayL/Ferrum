#pragma once

#include "common/common.h"
#include "codegen/AST.h"
#include "parser/types.h"

const char * llvm_ast_type_to_llvm_type(struct Ast * ast);
