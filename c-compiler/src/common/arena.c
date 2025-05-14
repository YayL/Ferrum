#include "common/arena.h"
#include "common/logger.h"
#include <stdlib.h>

#define ARENA_GET_INDEX(ARENA, INDEX) ARENA.arena + INDEX * ARENA.item_size
#define ARENA_INITIAL_CAPACITY 1
#define ARENA_GROWTH_RATE 2

struct Arena arena_init(size_t item_size) {
	ASSERT(item_size != 0, "Arena does not allow item_size=0");
	return (struct Arena) {
		.arena = NULL,
		.capacity = 0,
		.size = 0,
		.item_size = item_size,
	};
}

void arena_grow(struct Arena arena, size_t new_capacity) {
	if (new_capacity <= arena.capacity) {
		FATAL("Call to arena_grow does not grow arena");
	} else if (arena.arena == NULL) {
		arena.capacity = ARENA_INITIAL_CAPACITY;
		arena.arena = malloc(arena.item_size * arena.capacity);
	} else {
		arena.capacity = new_capacity;
		arena.arena = realloc(arena.arena, arena.item_size * arena.capacity);
	}
}

void * arena_get(struct Arena arena, size_t index) {
	if (arena.size <= index) {
		FATAL("Invalid arena({i}) index({i})", arena.size, index);
	}

	return ARENA_GET_INDEX(arena, index);
}

void * arena_next(struct Arena * arena) {
	if (arena->arena == NULL) {
		arena->capacity = ARENA_INITIAL_CAPACITY;
		arena->arena = malloc(arena->item_size * arena->capacity);
	} else if (arena->capacity <= arena->size) {
		arena->capacity *= ARENA_GROWTH_RATE;
		arena->arena = realloc(arena->arena, arena->item_size * arena->capacity);
	}

	return arena->arena + arena->item_size * arena->size++;
}
