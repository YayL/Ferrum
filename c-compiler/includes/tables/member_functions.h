#pragma once

#include "common/ID.h"
#include "common/memory/arena.h"

void member_function_index_init();

void member_function_index_add(ID name_id, ID member_function_id);
Arena member_function_index_lookup(ID name_id);
