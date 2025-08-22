#pragma once

#include "common/common.h"

typedef struct arena {
	void * arena;
	uint32_t capacity;
	uint32_t size;
	uint32_t item_size;
} Arena;

#define ARENA_APPEND(arena_ref, item) *((typeof(item) *) arena_next(arena_ref)) = (item)
#define ARENA_GET(arena, index, type) (*(type *) arena_get_ref(arena, index))
#define ARENA_POP(arena_ref, type) ARENA_GET(*arena_ref, (arena_ref)->size - 1, type); arena_shrink(arena_ref, 1)

Arena arena_init(uint32_t item_size);

void arena_grow(Arena * arena, uint32_t new_capacity);
void arena_shrink(Arena * arena, uint32_t shrink_amount);
void arena_clear(Arena * arena);

void * arena_get_ref(Arena arena, uint32_t index);
void * arena_next(Arena * arena);

void arena_extend(Arena * dest, const Arena src);
