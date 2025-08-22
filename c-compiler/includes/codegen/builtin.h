#pragma once

#include "codegen/AST.h"
#include "codegen/gen.h"
#include "codegen/llvm.h"

#define BUILTIN_FOR_EACH(f) \
	f(LLVM_STORE, "#llvm_store") \
	f(LLVM_LOAD, "#llvm_load") \
	f(LLVM_REGISTER_OF, "#llvm_register_of") \
	f(LLVM_GET_TYPE, "#llvm_get_type")

#define BUILTIN_ENUM_MEMBER(ENUM, ...) ENUM,
enum builtins {
	BUILTIN_FOR_EACH(BUILTIN_ENUM_MEMBER)
};

struct builtin {
	const char * str;
	unsigned int name_id;
	enum builtins builtin;
};

void builtin_intern();

char builtin_interner_id_is_inbounds(unsigned int ID);
struct builtin builtin_get_by_intern_id(unsigned int ID);

const char * gen_builtin(struct AST * ast, struct AST * self_type);
