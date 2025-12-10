#include "common/memory/arena.h"

#include "common/common.h"
#include "common/math.h"

#define ARENA_GET_INDEX(ARENA, INDEX) ARENA.arena + INDEX * ARENA.item_size
#define ARENA_INITIAL_CAPACITY 4
#define ARENA_GROWTH_RATE 2

Arena arena_init(uint32_t item_size) {
	ASSERT(item_size != 0, "Arena does not allow item_size=0");
	return (Arena) {
		.arena = NULL,
		.capacity = 0,
		.size = 0,
		.item_size = item_size,
	};
}

void arena_free(Arena arena) {
	free(arena.arena);
}

void arena_grow(Arena * arena, uint32_t new_capacity) {
	if (arena->arena == NULL) {
		arena->capacity = new_capacity;
		arena->arena = malloc(arena->item_size * arena->capacity);
	} else {
		ASSERT1(arena->arena != NULL);
		ASSERT1(arena->capacity < new_capacity);
		arena->capacity = new_capacity;
		arena->arena = realloc(arena->arena, arena->item_size * arena->capacity);
	}
	ASSERT1(arena->arena != NULL);
}

void arena_shrink(Arena * arena, uint32_t shrink_amount) {
	ASSERT1((int) arena->size - shrink_amount >= 0);
	arena->size -= shrink_amount;
}

void arena_clear(Arena * arena) {
	ASSERT1(arena != NULL);
	arena->size = 0;
}

void * arena_get_ref(Arena arena, uint32_t index) {
	if (arena.size <= index) {
		FATAL("Invalid arena({i}) index({i})", arena.size, index);
	}

	return ARENA_GET_INDEX(arena, index);
}

void * arena_next(Arena * arena) {
	if (arena->arena == NULL) {
		ASSERT1(arena->size == 0); // Arena should have size 0 if not allocated
		arena->capacity = ARENA_INITIAL_CAPACITY;
		arena->arena = malloc(arena->item_size * arena->capacity);
		ASSERT1(arena->arena != NULL);
	} else if (arena->capacity <= arena->size) {
		arena_grow(arena, arena->capacity * ARENA_GROWTH_RATE);
	}

	return arena->arena + arena->item_size * arena->size++;
}

void arena_extend(Arena * dest, const Arena src) {
	ASSERT1(dest != NULL);
	ASSERT(dest->item_size == src.item_size, "To extend an arena the item_sizes must be the same");
	if (src.size == 0) {
		return;
	}

	uint32_t needed_size = dest->size + src.size;
	if (needed_size == 0) {
		return;
	}

	if (dest->capacity < needed_size) {
		ASSERT(ARENA_GROWTH_RATE == 2, "Arena growth rate was adjusted, please fix this code");
		uint32_t adjusted_capacity = 1 << log2i(needed_size - 1); // subtact one to "ceil" result
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
