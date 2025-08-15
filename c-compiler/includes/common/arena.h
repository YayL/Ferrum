#pragma once

#include "common/common.h"

typedef struct arena {
	void * arena;
	size_t capacity;
	size_t size;
	size_t item_size;
} Arena;

#define ARENA_APPEND(arena_ref, item) *((typeof(item) *) arena_next(arena_ref)) = (item)
#define ARENA_FILL_FROM_LIST(arena_ref, list_ref) memcpy()

Arena arena_init(size_t item_size);

void arena_grow(Arena * arena, size_t new_capacity);
void arena_clear(Arena * arena);

void * arena_get(Arena arena, size_t index);
void * arena_next(Arena * arena);

void arena_extend(Arena * dest, const Arena src);
