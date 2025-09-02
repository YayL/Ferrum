#pragma once

#include <stdint.h>

struct buffer_blocks {
	char ** blocks;
	uint32_t block_count;
	uint32_t space_left_in_last_block;
};

char * buffer_blocks_allocate(uint32_t size);
