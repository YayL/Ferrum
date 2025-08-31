#include "common/block_arena.h"

#include "common/common.h"
#include <unistd.h>

const uint32_t blocksize = 4096;

#define INDEX_TO_BLOCK(INDEX, BLOCK_MAX_ITEM_COUNT) ((INDEX) / (BLOCK_MAX_ITEM_COUNT))
#define INDEX_TO_BLOCK_INDEX(INDEX, BLOCK_MAX_ITEM_COUNT) ((INDEX) % (BLOCK_MAX_ITEM_COUNT))

BArena block_arena_init(uint32_t item_size) {
	ASSERT1(blocksize > item_size);
	BArena barena = {
		.blocks = NULL,
		.item_count = 0,
		.item_size = item_size,
		.block_count = 0,
		.block_max_item_count = blocksize / item_size
	};

	block_arena_add_block(&barena);
	return barena;
}

void block_arena_add_block(BArena * barena) {
	if (barena->blocks == NULL) {
		barena->blocks = malloc(sizeof(*barena->blocks) * ++barena->block_count);
	} else {
		barena->blocks = realloc(barena->blocks, sizeof(*barena->blocks) * ++barena->block_count);
	}

	barena->blocks[barena->block_count - 1] = malloc(barena->item_size * barena->block_max_item_count);
}

void block_arena_remove(BArena * barena, uint32_t remove_count) {
	ASSERT1(barena != NULL);
	ASSERT1(barena->item_count >= remove_count);
	barena->item_count -= remove_count;
}

void * block_arena_get_ref(BArena barena, uint32_t index) {
	ASSERT1(barena.blocks != NULL);
	ASSERT1(barena.item_count > index);
	return barena.blocks[INDEX_TO_BLOCK(index, barena.block_max_item_count)] + (barena.item_size * INDEX_TO_BLOCK_INDEX(index, barena.block_max_item_count));
}

void * block_arena_next(BArena * barena) {
	ASSERT1(barena != NULL);
	ASSERT1(INDEX_TO_BLOCK(barena->item_count, barena->block_max_item_count) <= barena->block_count); // Skipped block???

	if (INDEX_TO_BLOCK(barena->item_count, barena->block_max_item_count) == barena->block_count) {
		block_arena_add_block(barena);
	}

	uint32_t index = barena->item_count++;
	return barena->blocks[INDEX_TO_BLOCK(index, barena->block_max_item_count)] + (barena->item_size * INDEX_TO_BLOCK_INDEX(index, barena->block_max_item_count));
}
