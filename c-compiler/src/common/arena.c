#include "common/arena.h"
#include "common/logger.h"
#include "common/math.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define ARENA_GET_INDEX(ARENA, INDEX) ARENA.arena + INDEX * ARENA.item_size
#define ARENA_INITIAL_CAPACITY 1
#define ARENA_GROWTH_RATE 2

Arena arena_init(size_t item_size) {
	ASSERT(item_size != 0, "Arena does not allow item_size=0");
	return (Arena) {
		.arena = NULL,
		.capacity = 0,
		.size = 0,
		.item_size = item_size,
	};
}

void arena_grow(Arena * arena, size_t new_capacity) {
	if (new_capacity <= arena->capacity) {
		FATAL("Call to arena_grow does not grow arena");
	} else if (arena->arena == NULL) {
		arena->capacity = ARENA_INITIAL_CAPACITY;
		arena->arena = malloc(arena->item_size * arena->capacity);
	} else {
		arena->capacity = new_capacity;
		arena->arena = realloc(arena->arena, arena->item_size * arena->capacity);
	}
}

void arena_clear(Arena * arena) {
	ASSERT1(arena != NULL);
	arena->size = 0;
}

void * arena_get(Arena arena, size_t index) {
	if (arena.size <= index) {
		FATAL("Invalid arena({i}) index({i})", arena.size, index);
	}

	return ARENA_GET_INDEX(arena, index);
}

void * arena_next(Arena * arena) {
	if (arena->arena == NULL) {
		arena->capacity = ARENA_INITIAL_CAPACITY;
		arena->arena = malloc(arena->item_size * arena->capacity);
	} else if (arena->capacity <= arena->size) {
		arena->capacity *= ARENA_GROWTH_RATE;
		arena->arena = realloc(arena->arena, arena->item_size * arena->capacity);
	}

	return arena->arena + arena->item_size * arena->size++;
}

void arena_extend(Arena * dest, const Arena src) {
	ASSERT1(dest != NULL);
	ASSERT(dest->item_size == src.item_size, "To extend an arena the item_sizes must be the same");
	if (src.size == 0) {
		return;
	}

	size_t needed_size = dest->size + src.size;
	if (needed_size == 0) {
		return;
	}

	if (dest->capacity < needed_size) {
		ASSERT(ARENA_GROWTH_RATE == 2, "Arena growth rate was adjusted, please fix this code");
		size_t adjusted_capacity = 1 << log2i(needed_size - 1); // subtact one to "ceil" result
		arena_grow(dest, needed_size);
	}

	memcpy(dest->arena + dest->item_size * dest->size, src.arena, dest->item_size * src.size);
}

void arena_extend_with_list(Arena * dest, const struct List * src) {
	Arena temp = {
		.arena = src->items,
		.size = src->size,
		.capacity = src->capacity,
		.item_size = src->item_size,
	};

	arena_extend(dest, temp);
}
