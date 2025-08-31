#pragma once

#include <stdint.h>

typedef struct block_arena {
	void ** blocks;
	uint32_t item_count;
	uint32_t item_size;
	uint16_t block_count;
	uint16_t block_max_item_count;
} BArena;

BArena block_arena_init(uint32_t item_size);

void block_arena_add_block(BArena * barena);
void block_arena_remove(BArena * barena, uint32_t remove_count);

void * block_arena_get_ref(BArena barena, uint32_t index);
void * block_arena_next(BArena * barena);
