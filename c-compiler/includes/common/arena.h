#pragma once

#include "common/common.h"

struct Arena {
	void * arena;
	size_t capacity;
	size_t size;
	size_t item_size;
};

#define ARENA_APPEND(arena_ref, item) *((typeof(item) *) arena_next(arena_ref)) = (item)

struct Arena arena_init(size_t item_size);

void arena_grow(struct Arena arena, size_t new_capacity);

void * arena_get(struct Arena arena, size_t index);
void * arena_next(struct Arena * arena);
