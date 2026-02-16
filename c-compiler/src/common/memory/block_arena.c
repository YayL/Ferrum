#include "common/memory/block_arena.h"

#include "common/common.h"
#include <unistd.h>

const uint32_t blocksize = 8192;

#define BARENA_INITIAL_BLOCK_COUNT 1

#define INDEX_TO_BLOCK(INDEX, BLOCK_MAX_ITEM_COUNT) ((INDEX) / (BLOCK_MAX_ITEM_COUNT))
#define INDEX_TO_BLOCK_INDEX(INDEX, BLOCK_MAX_ITEM_COUNT) ((INDEX) % (BLOCK_MAX_ITEM_COUNT))
#define BARENA_GET_REF(BARENA, INDEX) ((BARENA)->blocks[INDEX_TO_BLOCK(INDEX, (BARENA)->block_max_item_count)] + ((BARENA)->item_size * INDEX_TO_BLOCK_INDEX(INDEX, (BARENA)->block_max_item_count)))

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

void block_arena_free(BArena * barena) {
	ASSERT1(barena->block_count != 0);
	free(barena->blocks[0]);

	for (size_t i = 1; i < barena->block_count; i *= 2) {
		free(barena->blocks[i]);
	}

	free(barena->blocks);
}

void block_arena_add_block(BArena * barena) {
	ASSERT1(barena != NULL);

	size_t initial_block_count = 0;
	if (barena->blocks == NULL) {
		barena->block_count = BARENA_INITIAL_BLOCK_COUNT;
		barena->blocks = malloc(sizeof(*barena->blocks) * barena->block_count);
	} else {
		ASSERT1(barena->block_count != 0);
		initial_block_count = barena->block_count;
		barena->block_count = barena->block_count * 2;
		barena->blocks = realloc(barena->blocks, sizeof(*barena->blocks) * barena->block_count);
	}

	void * new_blocks_start = malloc((barena->block_count - initial_block_count) * barena->item_size * barena->block_max_item_count);

	for (size_t i = initial_block_count; i < barena->block_count; ++i) {
		barena->blocks[i] = new_blocks_start + (i - initial_block_count) * barena->item_size * barena->block_max_item_count;
	}
}

void block_arena_remove(BArena * barena, uint32_t remove_count) {
	ASSERT1(barena != NULL);
	ASSERT1(barena->item_count >= remove_count);
	barena->item_count -= remove_count;
}

void * block_arena_get_ref(BArena barena, uint32_t index) {
	ASSERT1(barena.blocks != NULL);
	ASSERT1(barena.item_count > index);
	return BARENA_GET_REF(&barena, index);
}

void * block_arena_next(BArena * barena) {
	ASSERT1(barena != NULL);
	ASSERT1(INDEX_TO_BLOCK(barena->item_count, barena->block_max_item_count) <= barena->block_count); // Skipped block???

	if (INDEX_TO_BLOCK(barena->item_count, barena->block_max_item_count) == barena->block_count) {
		block_arena_add_block(barena);
	}

	uint32_t index = barena->item_count++;
	return BARENA_GET_REF(barena, index);
}
