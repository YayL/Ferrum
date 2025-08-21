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

Arena arena_init(uint32_t item_size);

void arena_grow(Arena * arena, uint32_t new_capacity);
void arena_clear(Arena * arena);

void * arena_get_ref(Arena arena, uint32_t index);
void * arena_next(Arena * arena);

void arena_extend(Arena * dest, const Arena src);
