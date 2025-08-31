#pragma once

#include "common/ID.h"

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
	ID name_id;
	enum builtins builtin;
};

void builtin_intern();

char builtin_interner_id_is_inbounds(ID id);
struct builtin builtin_get_by_intern_id(ID id);

const char * gen_builtin(ID node_id);
