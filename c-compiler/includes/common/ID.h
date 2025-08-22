#pragma once

#include "common/hashmap.h"
#include "common/logger.h"

#define COMPILER_ID_TYPE unsigned int

#define ID_FOR_EACH(f) \
	f(ID_INTERNER, "interner") \
	f(ID_MODULE, "module") \
	f(ID_TYPE, "type") 

#define ID_TYPE_ENUM_MEMBER(TYPE, STR) TYPE,

typedef struct id {
	COMPILER_ID_TYPE id;
	enum id_type {
		ID_INVALID_TYPE = 0,
		ID_FOR_EACH(ID_TYPE_ENUM_MEMBER)
	} type;
} ID;

#define INVALID_ID ((ID) { .type = ID_INVALID_TYPE, .id = 0 })
#define ID_IS_INVALID(ID) (ID.type == ID_INVALID_TYPE)

static kh_inline khint_t _id_hash(ID id) {
	return id.id;
}

static kh_inline char _id_is_equal(ID id1, ID id2) {
	ASSERT1(id1.type == id2.type);
	return id1.id == id2.id;
}

KHASH_MAP_INIT_STR(map_string_to_id, ID);
KHASH_INIT(map_id_to_id, ID, ID, KHASH_IS_MAP, _id_hash, _id_is_equal);

static inline ID id_init(COMPILER_ID_TYPE id, enum id_type type) {
	return (ID) { .id = id, .type = type };
}

static inline char id_is_equal(ID id1, ID id2) {
	return id1.type == id2.type && id1.id == id2.id;
}


