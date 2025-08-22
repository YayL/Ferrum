#include "tables/registry.h"
#include "common/defines.h"
#include "common/hashmap.h"
#include "common/logger.h"

#define REGISTRY_ID_TO_INDEX(ID) (ID.id - 1)

struct registry registry_init(unsigned int item_size) {
	return (struct registry) {
		.entries = arena_init(item_size),	// store at (registry_id - 1)
		.map = kh_init(map_id_to_id),		// store key -> registry_id
	};
}

void * registry_insert(struct registry * registry, ID key, ID * previous_value) {
	int ret_code;
	khint_t k = kh_put(map_id_to_id, &registry->map, key, &ret_code);
	ASSERT1(k != kh_end(&registry->map)); // something weird occured
	ASSERT(ret_code == 1, "Some error occured while retrieving from registry hashmap. Error code {i}", ret_code);

	if (previous_value != NULL && ret_code == KEY_ALREADY_PRESENT) {
		*previous_value = kh_value(&registry->map, k);
	}

	kh_value(&registry->map, k) = registry->entries.size + 1;
	return arena_next(&registry->entries);
}

void registry_remove(struct registry * registry, ID key, char (*callback_remove_predicate)(ID, void *)) {
	khint_t k = kh_get(map_id_to_id, &registry->map, key);
	if (k == kh_end(&registry->map)) {
		FATAL("Attempt to remove unregistered registry ID");
	}

	ID id = kh_value(&registry->map, k);
	ASSERT(id.id == registry->entries.size, "Only allowed to remove the latest recorded registry ID");

	if (callback_remove_predicate == NULL || callback_remove_predicate(id, ARENA_GET(registry->entries, REGISTRY_ID_TO_INDEX(id), void *))) {
		kh_del(map_id_to_id, &registry->map, k); // Unregister key
	}

	arena_shrink(&registry->entries, 1);
}

ID registry_lookup(struct registry registry, ID key) {
	khint_t k = kh_get(map_id_to_id, &registry.map, key);

	if (k == kh_end(&registry.map)) {
		return INVALID_ID;
	}

	return kh_value(&registry.map, k);
}
