#pragma once

#include "common/common.h"
#include "codegen/AST.h"
#include "parser/types.h"

const char * llvm_type_to_llvm_type(Type * type, struct Ast * self_type);

const char * llvm_ast_type_to_llvm_type(struct Ast * ast, struct Ast * self_type);

const char * llvm_ast_type_to_llvm_arg_type(struct Ast * ast, struct Ast * self_type);

unsigned int llvm_get_register_of(struct Ast * ast);
