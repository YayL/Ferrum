#pragma once

#include "common/data/hashmap.h"
#include "common/logger.h"

#define OTHER_REGISTRY_KINDS(f) \
	f(ID_INTERNER,			"InternerID",		interner_entry,			INTERNER) \
	f(ID_SYMBOL,			"SymbolID",			symbol_map_entry,		SYMBOL)

#define TYPE_CHECKING_REGISTRY_KINDS(f) \
	f(ID_TC_CONSTRAINT,		"ConstraintID",		Constraint_TC,		TC)	\
	f(ID_TC_DIMENSION,		"DimensionID",		Dimension_TC,		TC)	\
	f(ID_TC_GENERIC,		"GenericID",		Generic_TC,			TC)	\
	f(ID_TC_SHAPE,			"ShapeId",			Shape_TC,			TC)	\
	f(ID_TC_VARIABLE,		"VariableID",		Variable_TC,		TC)

#define TYPE_REGISTRY_KINDS(f) \
	f(ID_NUMERIC_TYPE,	"NumericT",		Numeric_T,	TYPE) \
	f(ID_SYMBOL_TYPE,	"SymbolT",		Symbol_T,	TYPE) \
	f(ID_REF_TYPE,		"RefT",			Ref_T,		TYPE) \
	f(ID_ARRAY_TYPE,	"ArrayT",		Array_T,	TYPE) \
	f(ID_TUPLE_TYPE,	"TupleT",		Tuple_T,	TYPE) \
	f(ID_PLACE_TYPE,	"PlaceT",		Place_T,	TYPE) \
	f(ID_FN_TYPE,		"FnT",			Fn_T,		TYPE)

#define AST_REGISTRY_KINDS(f) \
    f(ID_AST_MODULE,		"Module",			a_module,				AST) \
    f(ID_AST_IMPORT,		"Import",			a_import,				AST) \
    f(ID_AST_FUNCTION,		"Function",			a_function,				AST) \
    f(ID_AST_SCOPE,			"Scope",			a_scope,				AST) \
    f(ID_AST_DECLARATION,	"Declaration",		a_declaration,			AST) \
    f(ID_AST_EXPR,			"Expression",		a_expression,			AST) \
    f(ID_AST_OP,			"Operator",			a_operator,				AST) \
    f(ID_AST_SYMBOL,		"Symbol",			a_symbol,				AST) \
    f(ID_AST_VARIABLE,		"Variable",			a_variable,				AST) \
    f(ID_AST_LITERAL,		"Literal",			a_literal,				AST) \
    f(ID_AST_TUPLE,			"Tuple",			a_tuple,				AST) \
    f(ID_AST_ARRAY,			"Array",			a_array,				AST) \
    f(ID_AST_FOR,			"For",				a_for_statement,		AST) \
    f(ID_AST_RETURN,		"Return",			a_return_statement,		AST) \
    f(ID_AST_WHILE,			"While",			a_while_statement,		AST) \
    f(ID_AST_IF,			"If",				a_if_statement,			AST) \
    f(ID_AST_MATCH,			"Match",			a_match_statement,		AST) \
    f(ID_AST_DO,			"Do",				a_do_statement,			AST) \
    f(ID_AST_BREAK,			"Break",			a_break_statement,		AST) \
    f(ID_AST_CONTINUE,		"Continue",			a_continue_statement,	AST) \
    f(ID_AST_STRUCT,		"Struct",			a_structure,			AST) \
    f(ID_AST_ENUM,			"Enum",				a_enumeration,			AST) \
    f(ID_AST_TRAIT,			"Trait",			a_trait,				AST) \
    f(ID_AST_IMPL,			"Implementation",	a_implementation,		AST)

#define REGISTRY_KINDS(f) \
	OTHER_REGISTRY_KINDS(f) \
	TYPE_CHECKING_REGISTRY_KINDS(f) \
	TYPE_REGISTRY_KINDS(f) \
	AST_REGISTRY_KINDS(f)

#define COMPILER_ID_TYPE unsigned int
#define ID_TYPE_ENUM_MEMBER(ENUM, ...) ENUM,

#define BIT_SIZE_ID_TYPE 6
typedef struct id {
	COMPILER_ID_TYPE id : (sizeof(COMPILER_ID_TYPE) * 8 - BIT_SIZE_ID_TYPE);
	enum id_type {
		ID_INVALID_TYPE = 0,
		ID_VOID_TYPE,
		ID_AST_ROOT,
		REGISTRY_KINDS(ID_TYPE_ENUM_MEMBER)
		ID_LARGEST
	} type : BIT_SIZE_ID_TYPE;
} ID;

#define INVALID_ID ((ID) { .type = ID_INVALID_TYPE, .id = 0 })
#define ID_IS_INVALID(ID) (ID.type == ID_INVALID_TYPE)
#define ID_IS(ID, TYPE) (ID.type == TYPE)

static inline ID id_init(COMPILER_ID_TYPE id, enum id_type type) {
	return (ID) { .id = id, .type = type };
}

static inline char id_is_equal(ID id1, ID id2) {
	return id1.type == id2.type && id1.id == id2.id;
}

#define _ID_GEN_ID_TO_TYPE_STRING_CASE(ENUM, STR, ...) case ENUM: return STR;
static const char * id_type_to_string(enum id_type type) {
	switch (type) {
		case ID_INVALID_TYPE: return "INVALID";
		case ID_VOID_TYPE: return "VoidT";
		case ID_AST_ROOT: return "ROOT";
		REGISTRY_KINDS(_ID_GEN_ID_TO_TYPE_STRING_CASE)
	}

	return format("{i} (unknown)", type);
}

static kh_inline khint_t _id_hash(ID id) {
	return kh_int_hash_func(id.id);
}

static kh_inline char _id_is_equal(ID id1, ID id2) {
	ASSERT1(id1.type == id2.type);
	return id1.id == id2.id;
}

KHASH_MAP_INIT_STR(map_string_to_id, ID);
KHASH_INIT(map_id_to_id, ID, ID, KHASH_IS_MAP, _id_hash, _id_is_equal);
