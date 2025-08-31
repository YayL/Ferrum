#pragma once

#include "common/block_arena.h"
#include "common/ID.h"
#include "common/logger.h"

typedef struct registry {
	BArena entries;
	enum id_type valid_type;
} Registry;

static inline struct registry registry_init(enum id_type valid_type, unsigned int item_size) {
	return (struct registry) {
		.entries = block_arena_init(item_size),
		.valid_type = valid_type // the ID type used for lookup
	};
}

#include "tables/interner.h"
static inline void * registry_lookup(struct registry registry, ID key) {
	ASSERT1(key.type == registry.valid_type);
	ASSERT1(key.id > 0);
	return block_arena_get_ref(registry.entries, key.id - 1);
}

static inline void * registry_allocate(struct registry * registry, ID * key) {
	key->id = registry->entries.item_count + 1;
	key->type = registry->valid_type;

	// return block_arena_next(&registry->entries);
	block_arena_next(&registry->entries);
	return registry_lookup(*registry, *key);
}

static inline void registry_remove(struct registry * registry, ID key) {
	ASSERT1(registry->valid_type == key.type);
	ASSERT1(registry->entries.item_count == key.id);
	block_arena_remove(&registry->entries, 1);
}
