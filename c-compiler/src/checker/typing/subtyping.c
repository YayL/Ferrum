#include "checker/typing/subtyping.h"

#include "common/ID.h"
#include "parser/types.h"
#include "tables/registry_manager.h"
#include "checker/context.h"
#include "checker/typing/utils.h"

char instantiate(Solver * solver, ID var_id, ID type_id) {
	ASSERT1(ID_IS(var_id, ID_EXISTENIAL));

	if (!is_free_from_variable(type_id, var_id)) {
		return 0;
	}

	return instantiate_solve(solver, var_id, type_id);
}

char instaniate_tuple(Solver * solver, ID tuple_id, ID expected_id) {
	ASSERT1(ID_IS(tuple_id, ID_TUPLE_TYPE));

	Tuple_T tuple = LOOKUP(tuple_id, Tuple_T);
	if (tuple.types.size == 1) {
		if (ID_IS(expected_id, ID_TUPLE_TYPE)) {
			return is_subtype(solver, ARENA_GET(tuple.types, 0, ID), ARENA_GET(LOOKUP(expected_id, Tuple_T).types, 0, ID));
		}

		return is_subtype(solver, ARENA_GET(tuple.types, 0, ID), expected_id);
	}

	Tuple_T expected_tuple = LOOKUP(expected_id, Tuple_T);
	if (tuple.types.size != expected_tuple.types.size) {
		return 0;
	}

	for (size_t i = 0; i < tuple.types.size; ++i) {
		ID A = ARENA_GET(tuple.types, i, ID), B = ARENA_GET(expected_tuple.types, i, ID);
		if (!is_subtype(solver, A, B)) {
			// println("{s} <!: {s}", type_to_str(A), type_to_str(B));
			return 0;
		}
		// println("{s} <: {s}", type_to_str(A), type_to_str(B));
	}

	return 1;
}

char is_subtype_strict(Solver * solver, ID lower, ID upper) {
	if (ID_IS_EQUAL(lower, upper)) {
		return 1;
	}

	lower = solver_deflate_type(lower), upper = solver_deflate_type(upper);

	// println("{s} <?: {s}", type_to_str(lower), type_to_str(upper));

	if (ID_IS_EQUAL(lower, upper)) {
		return 1;
	}

	if (ID_IS(lower, ID_EXISTENIAL)) {
		return instantiate(solver, lower, upper);
	} else if (ID_IS(upper, ID_EXISTENIAL)) {
		return instantiate(solver, upper, lower);
	}

	if (ID_IS(lower, ID_TUPLE_TYPE)) {
		return instaniate_tuple(solver, lower, upper);
	} else if (ID_IS(upper, ID_TUPLE_TYPE)) {
		return instaniate_tuple(solver, upper, lower);
	}

	if (id_is_type(lower) && id_is_type(upper)) {
		return lower.type == upper.type && subtype_check_equal(solver, lower, upper);
	}

	FATAL("Unimplemented: [{s}] [{s}]", type_to_str(lower), type_to_str(upper));
}

char is_subtype(Solver * solver, ID lower, ID upper) {
	return is_subtype_strict(solver, lower, upper) || is_implicit_subtype(solver, lower, upper);
}

char instantiate_solve(Solver * solver, ID A, ID B) {
	ID * dest = NULL, src = INVALID_ID;

	if (ID_IS(A, ID_EXISTENIAL) && ID_IS(B, ID_EXISTENIAL)) {
		Existential * existential_a = lookup(A),
					* existential_b = lookup(B);

		if (ID_IS_INVALID(existential_a->solved_type) && ID_IS_INVALID(existential_b->solved_type)) {
			FATAL("Unimplemented");
		} else if (ID_IS_INVALID(existential_a->solved_type)) {
			dest = &existential_a->solved_type, src = B;
		} else if (ID_IS_INVALID(existential_b->solved_type)) {
			dest = &existential_b->solved_type, src = A;
		} else {
			return is_subtype(solver, existential_a->solved_type, existential_b->solved_type);
		}
	} else if (ID_IS(A, ID_EXISTENIAL)) {
		dest = &(LOOKUP(A, Existential).solved_type), src = B;
	} else if (ID_IS(B, ID_EXISTENIAL)) {
		dest = &(LOOKUP(B, Existential).solved_type), src = A;
	}

	ASSERT1(dest != NULL);
	ASSERT1(!ID_IS_INVALID(src));

	if (ID_IS_INVALID(*dest)) {
		if (ID_IS(src, ID_PLACE_TYPE)) {
			src = LOOKUP(src, Place_T).basetype_id;
		}
		*dest = src;
	} else {
		return is_subtype(solver, *dest, src);
	}

	return 1;
}

char subtype_check_equal(Solver * solver, ID type_id1, ID type_id2) {
	if (type_id1.type != type_id2.type) {
		return 0;
	}

	switch (type_id1.type) {
		case ID_PLACE_TYPE: {
            Place_T place1 = LOOKUP(type_id1, Place_T), place2 = LOOKUP(type_id2, Place_T);
			return place1.is_mut == place2.is_mut && is_subtype(solver, place1.basetype_id, place2.basetype_id);
		}
		case ID_REF_TYPE: {
			Ref_T ref1 = LOOKUP(type_id1, Ref_T), ref2 = LOOKUP(type_id2, Ref_T);
			return ref1.is_mut == ref2.is_mut && ref1.depth == ref2.depth && is_subtype(solver, ref1.basetype_id, ref2.basetype_id);
		}
		case ID_ARRAY_TYPE: {
			Array_T arr1= LOOKUP(type_id1, Array_T), arr2 = LOOKUP(type_id2, Array_T);
			return arr1.size == arr2.size && is_subtype(solver, arr1.basetype_id, arr2.basetype_id);
		}
		case ID_SYMBOL_TYPE: {
			Symbol_T symbol_type1 = LOOKUP(type_id1, Symbol_T), symbol_type2 = LOOKUP(type_id2, Symbol_T);

            if (symbol_type1.templates.size != symbol_type2.templates.size) {
                return 0;
            }

			a_symbol * symbol1 = lookup(symbol_type1.symbol_id), * symbol2 = lookup(symbol_type2.symbol_id);
            ASSERT1(!ID_IS_INVALID(symbol1->node_id));
            ASSERT1(!ID_IS_INVALID(symbol2->node_id));

            if (!ID_IS_EQUAL(symbol1->node_id, symbol2->node_id)) {
                return 0;
            }

            for (size_t i = 0; i < symbol_type1.templates.size; ++i) {
                if (!is_subtype(solver, ARENA_GET(symbol_type1.templates, i, ID), ARENA_GET(symbol_type2.templates, i, ID))) {
                    return 0;
                }
            }

            return 1;
		}
        case ID_NUMERIC_TYPE: {
            Numeric_T num1 = LOOKUP(type_id1, Numeric_T), num2 = LOOKUP(type_id2, Numeric_T);
            return num1.type == num2.type && num1.width == num2.width;
        }
        case ID_FN_TYPE: {
            Fn_T fn1 = LOOKUP(type_id1, Fn_T), fn2 = LOOKUP(type_id2, Fn_T);
            return subtype_check_equal(solver, fn1.ret_type, fn2.ret_type) && subtype_check_equal(solver, fn1.arg_type, fn2.arg_type);
        }
        case ID_TUPLE_TYPE: {
			FATAL("NO");
            // if (ID_IS_EQUAL(type_id1, type_id2)) {
            //     return 1;
            // }
            //
            // Tuple_T tuple1 = LOOKUP(type_id1, Tuple_T), tuple2 = LOOKUP(type_id2, Tuple_T);
            // if (tuple1.types.size != tuple2.types.size) {
            //     return 0;
            // }
            //
            // for (size_t i = 0; i < tuple1.types.size; ++i) {
            //     if (!subtype_check_equal(ARENA_GET(tuple1.types, i, ID), ARENA_GET(tuple2.types, i, ID))) {
            //         return 0;
            //     }
            // }
            //
            // return 1;
        }
        case ID_VOID_TYPE: return 1;
		default:
			FATAL("Unimplemented id {s}", id_type_to_string(type_id1.type));
	}
}

char is_implicit_subtype(Solver * solver, ID lower, ID upper) {
	const a_trait imp_trait = LOOKUP(context_get_implicit_cast_trait(), a_trait);
	Arena arena = arena_init(sizeof(ID));
	arena_grow(&arena, 2);
	ARENA_APPEND(&arena, lower);
	ARENA_APPEND(&arena, upper);

	size_t count = 0;
	for (size_t i = 0; i < imp_trait.implementations.size; ++i) {
		ID implementation_id = ARENA_GET(imp_trait.implementations, i, ID);

		if (solver_implementation_is_valid(solver, implementation_id, arena)) {
			count += 1;
		}
	}

	ASSERT1(count <= 1);

	return count == 1;
}
