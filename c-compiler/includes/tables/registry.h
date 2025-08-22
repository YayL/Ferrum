#pragma once

#include "common/arena.h"
#include "common/ID.h"
#include "common/defines.h"

#define INVALID_REGISTRY_ID 0

struct registry {
	Arena entries;
	khash_t(map_id_to_id) map;
};

struct registry registry_init(unsigned int item_size);

void * registry_insert(struct registry * registry, ID key, ID * previous_value);
void registry_remove(struct registry * registry, ID key, char (*callback_remove_predicate)(ID, void *));

ID registry_lookup(struct registry registry, ID key);
