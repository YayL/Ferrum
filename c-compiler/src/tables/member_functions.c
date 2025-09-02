#include "tables/member_functions.h"

KHASH_INIT(map_id_to_arena, ID, Arena, KHASH_IS_MAP, _id_hash, _id_is_equal);

struct member_function_index {
	khash_t(map_id_to_arena) map;
} member_function_index;

void member_function_index_init() {
	member_function_index.map = kh_init(map_id_to_arena);
}

void member_function_index_add(ID name_id, ID member_function_id) {
	int retcode;
	khint_t k = kh_put(map_id_to_arena, &member_function_index.map, name_id, &retcode);

	Arena arena;
	if (retcode == KH_PUT_ALREADY_PRESENT) {
		arena = kh_value(&member_function_index.map, k);
	} else {
		arena = arena_init(sizeof(ID));
	}

	ARENA_APPEND(&arena, member_function_id);

	kh_value(&member_function_index.map, k) = arena;
}

Arena member_function_index_lookup(ID name_id) {
	khint_t k = kh_get(map_id_to_arena, &member_function_index.map, name_id);

	if (k == kh_end(&member_function_index.map)) {
		return (Arena) {0};
	}

	return kh_value(&member_function_index.map, k);
}
