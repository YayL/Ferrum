#pragma once

#include "common/ID.h"
#include "tables/registry_manager.h"
#include "parser/types.h"

static char is_free_from_variable(ID type_id, ID var_id) {
	if (ID_IS_EQUAL(type_id, var_id)) {
		return 0;
	}

	switch (type_id.type) {
		case ID_EXISTENIAL:
		case ID_VOID_TYPE:
		case ID_NUMERIC_TYPE: return 1;
		case ID_REF_TYPE: return is_free_from_variable(LOOKUP(type_id, Ref_T).basetype_id, var_id);
		case ID_ARRAY_TYPE: return is_free_from_variable(LOOKUP(type_id, Array_T).basetype_id, var_id);
		case ID_PLACE_TYPE: return is_free_from_variable(LOOKUP(type_id, Place_T).basetype_id, var_id);
		case ID_TUPLE_TYPE: {
			Tuple_T tuple = LOOKUP(type_id, Tuple_T);
			for (size_t i = 0; i < tuple.types.size; ++i) {
				if (!is_free_from_variable(ARENA_GET(tuple.types, i, ID), var_id)) {
					return 0;
				}
			}
		} break;
		default: FATAL("Unimplemented type \"{s}\"", id_type_to_string(type_id.type));
	}

	return 1;
}

static char is_free_from_existentials(ID type_id) {
	switch (type_id.type) {
		case ID_EXISTENIAL: {
			Existential existential = LOOKUP(type_id, Existential);
			if (ID_IS_INVALID(existential.solved_type)) {
				return 0;
			}

			return is_free_from_existentials(existential.solved_type);
		}
		case ID_VOID_TYPE:
		case ID_NUMERIC_TYPE: return 1;
		case ID_REF_TYPE: return is_free_from_existentials(LOOKUP(type_id, Ref_T).basetype_id);
		case ID_ARRAY_TYPE: return is_free_from_existentials(LOOKUP(type_id, Array_T).basetype_id);
		case ID_PLACE_TYPE: return is_free_from_existentials(LOOKUP(type_id, Place_T).basetype_id);
		case ID_FN_TYPE: {
			Fn_T fn_type = LOOKUP(type_id, Fn_T);
			return is_free_from_existentials(fn_type.arg_type) && is_free_from_existentials(fn_type.ret_type);
		}
		case ID_TUPLE_TYPE: {
			Tuple_T tuple_type = LOOKUP(type_id, Tuple_T);
			for (size_t i = 0; i < tuple_type.types.size; ++i) {
				if (!is_free_from_existentials(ARENA_GET(tuple_type.types, i, ID))) {
					return 0;
				}
			}

			return 1;
		}
		case ID_SYMBOL_TYPE: {
			Symbol_T symbol_type = LOOKUP(type_id, Symbol_T);
			for (size_t i = 0; i < symbol_type.templates.size; ++i) {
				if (!is_free_from_existentials(ARENA_GET(symbol_type.templates, i, ID))) {
					return 0;
				}
			}

			return 1;
		}
		default: FATAL("Unimplemented type \"{s}\"", id_type_to_string(type_id.type));
	}
}
