#include "common/buffer.h"

#include "common/common.h"

#define BUFFER_BLOCK_SIZE 8000 * 1000 // 8MB
struct buffer_blocks buffer = {0};

void buffer_blocks_add_block() {
	if (buffer.blocks == NULL) {
		buffer.block_count = 1;
		buffer.blocks = malloc(sizeof(*buffer.blocks));
	} else {
		buffer.block_count += 1;
		buffer.blocks = realloc(buffer.blocks, sizeof(*buffer.blocks) * buffer.block_count);
	}

	buffer.blocks[buffer.block_count - 1] = malloc(sizeof(**buffer.blocks) * BUFFER_BLOCK_SIZE);
	buffer.space_left_in_last_block = BUFFER_BLOCK_SIZE;
}

char * buffer_blocks_allocate(uint32_t size) {
	if (buffer.blocks == NULL || buffer.space_left_in_last_block < size) {
		buffer_blocks_add_block();
	}

	buffer.space_left_in_last_block -= size;
	return &buffer.blocks[buffer.block_count - 1][(BUFFER_BLOCK_SIZE - (buffer.space_left_in_last_block + size))];
}
