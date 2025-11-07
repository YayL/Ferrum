#pragma once

#include "common/ID.h"

#define BUILTIN_FOR_EACH(f) \
	f(LLVM_STORE, "#llvm_store") \
	f(LLVM_LOAD, "#llvm_load") \
	f(LLVM_TYPE_OF, "#llvm_type_of") \
	f(LLVM_OP, "#llvm_op") \
	f(LLVM_ADDRESS_TO_PLACE, "#llvm_address_to_place") \
	f(LLVM_PLACE_TO_ADDRESS, "#llvm_place_to_address")

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
