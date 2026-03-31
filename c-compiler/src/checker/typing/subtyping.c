#include "checker/typing/subtyping.h"

#include "common/ID.h"
#include "parser/types.h"
#include "tables/registry_manager.h"

char is_free_from_variable(ID type, ID var_id) {
	if (ID_IS_EQUAL(type, var_id)) {
		return 0;
	}

	switch (type.type) {
		case ID_EXISTENIAL:
		case ID_NUMERIC_TYPE: return 1;
		case ID_REF_TYPE: return is_free_from_variable(LOOKUP(type, Ref_T).basetype_id, var_id);
		case ID_ARRAY_TYPE: return is_free_from_variable(LOOKUP(type, Array_T).basetype_id, var_id);
		case ID_PLACE_TYPE: return is_free_from_variable(LOOKUP(type, Place_T).basetype_id, var_id);
		case ID_TUPLE_TYPE: {
			Tuple_T tuple = LOOKUP(type, Tuple_T);
			for (size_t i = 0; i < tuple.types.size; ++i) {
				if (!is_free_from_variable(ARENA_GET(tuple.types, i, ID), var_id)) {
					return 0;
				}
			}
		} break;
		
		default: FATAL("Unimplemented type \"{s}\"", id_type_to_string(type.type));
	}

	return 1;
}

char instantiate(ID var_id, ID type_id) {
	ASSERT1(ID_IS(var_id, ID_EXISTENIAL));

	if (!is_free_from_variable(type_id, var_id)) {
		return 0;
	}

	instantiate_solve(var_id, type_id);
	return 1;
}

char instaniate_tuple(ID tuple_id, ID expected_id) {
	ASSERT1(ID_IS(tuple_id, ID_TUPLE_TYPE));

	Tuple_T tuple = LOOKUP(tuple_id, Tuple_T);
	if (tuple.types.size == 1) {
		if (ID_IS(expected_id, ID_TUPLE_TYPE)) {
			return is_subtype(ARENA_GET(tuple.types, 0, ID), ARENA_GET(LOOKUP(expected_id, Tuple_T).types, 0, ID));
		}

		return is_subtype(ARENA_GET(tuple.types, 0, ID), expected_id);
	}

	Tuple_T expected_tuple = LOOKUP(expected_id, Tuple_T);
	if (tuple.types.size != expected_tuple.types.size) {
		return 0;
	}

	for (size_t i = 0; i < tuple.types.size; ++i) {
		ID A = ARENA_GET(tuple.types, i, ID), B = ARENA_GET(expected_tuple.types, i, ID);
		if (!is_subtype(A, B)) {
			println("{s} <!: {s}", type_to_str(A), type_to_str(B));
			return 0;
		}
		println("{s} <: {s}", type_to_str(A), type_to_str(B));
	}

	return 1;
}

char is_subtype(ID A, ID B) {
	if (ID_IS_EQUAL(A, B)) {
		return 1;
	}

	if (ID_IS(A, ID_EXISTENIAL)) {
		return instantiate(A, B);
	} else if (ID_IS(B, ID_EXISTENIAL)) {
		return instantiate(B, A);
	}

	if (ID_IS(A, ID_TUPLE_TYPE)) {
		return instaniate_tuple(A, B);
	} else if (ID_IS(B, ID_TUPLE_TYPE)) {
		return instaniate_tuple(B, A);
	}

	return A.type == B.type && type_check_deep_equal(A, B);
}

void instantiate_solve(ID A, ID B) {
	if (ID_IS(A, ID_EXISTENIAL)) {
		(&LOOKUP(A, Existential))->solved_type = B;
	} else if (ID_IS(B, ID_EXISTENIAL)) {
		(&LOOKUP(B, Existential))->solved_type = A;
	}
}
