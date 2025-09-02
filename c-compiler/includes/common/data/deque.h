#pragma once

#define TYPE ID
#define INITIAL_CAPACITY (2)
#define GROWTH_SPEED 2

#define DEQUE_T(TYPE) struct deque_##TYPE
#define DEQUE_INIT(TYPE) deque_init_##TYPE()
#define DEQUE_FREE(TYPE, deque_ref) deque_free_##TYPE(deque_ref)
#define DEQUE_EXPAND(TYPE, deque_ref) deque_expand_##TYPE(deque_ref)
#define DEQUE_PUSH_BACK(TYPE, deque_ref, item) deque_push_back_##TYPE(deque_ref, item)
#define DEQUE_PUSH_FRONT(TYPE, deque_ref, item) deque_push_front_##TYPE(deque_ref, item)
#define DEQUE_POP_BACK(TYPE, deque_ref) deque_pop_back_##TYPE(deque_ref)
#define DEQUE_POP_FRONT(TYPE, deque_ref) deque_pop_front_##TYPE(deque_ref)
#define DEQUE_FRONT(TYPE, deque_ref) deque_front_##TYPE(deque_ref)
#define DEQUE_BACK(TYPE, deque_ref) deque_back_##TYPE(deque_ref)

#define IMPLEMENT_DEQUE(TYPE) \
	DEQUE_T(TYPE) { \
		TYPE * items; \
		unsigned int start; \
		unsigned int end; \
		unsigned int size; \
		unsigned int capacity; \
	}; \
	\
	static DEQUE_T(TYPE) DEQUE_INIT(TYPE) { return (DEQUE_T(TYPE)) { .capacity = INITIAL_CAPACITY, .start = 0, .end = 0, .size = 0, .items = malloc(INITIAL_CAPACITY * sizeof(TYPE)) }; } \
	static void DEQUE_FREE(TYPE, DEQUE_T(TYPE) deque) { free(deque.items); } \
	\
	static void DEQUE_EXPAND(TYPE, DEQUE_T(TYPE) * deque) { \
		unsigned int previous_cap = deque->capacity; \
		deque->capacity = deque->capacity * GROWTH_SPEED; \
		deque->items = realloc(deque->items, deque->capacity * sizeof(TYPE)); \
		\
		if (deque->end < deque->start) { \
			unsigned int length = previous_cap - deque->start; \
			memmove(deque->items + deque->capacity - length, deque->items + deque->start, length * sizeof(TYPE)); \
			deque->start = deque->capacity - length; \
		} \
	} \
	\
	static void DEQUE_PUSH_BACK(TYPE, DEQUE_T(TYPE) * deque, TYPE item) { \
		if (++deque->size == deque->capacity) DEQUE_EXPAND(TYPE, deque); \
		\
		deque->items[deque->end] = item; \
		deque->end = (deque->end + deque->capacity + 1) & (deque->capacity - 1); \
	} \
	\
	static void DEQUE_PUSH_FRONT(TYPE, DEQUE_T(TYPE) * deque, TYPE item) { \
		if (++deque->size == deque->capacity) DEQUE_EXPAND(TYPE, deque); \
		\
		deque->start = (deque->start + deque->capacity - 1) & (deque->capacity - 1); \
		deque->items[deque->start] = item; \
	} \
	\
	static void DEQUE_POP_BACK(TYPE, DEQUE_T(TYPE) * deque) { \
		if (!deque->size) return; \
		\
		deque->size -= 1; \
		deque->end = (deque->end + deque->capacity - 1) & (deque->capacity - 1); \
	} \
	\
	static void DEQUE_POP_FRONT(TYPE, DEQUE_T(TYPE) * deque) { \
		if (!deque->size) return; \
		\
		deque->size -= 1; \
		deque->start = (deque->start + deque->capacity + 1) & (deque->capacity - 1); \
	} \
	\
	static TYPE DEQUE_FRONT(TYPE, DEQUE_T(TYPE) * deque) { \
		if (!deque->size) return (TYPE) {0}; \
		\
		return deque->items[deque->start]; \
	} \
	\
	static TYPE DEQUE_BACK(TYPE, DEQUE_T(TYPE) * deque) { \
		if (!deque->size) return (TYPE) {0}; \
		\
		return deque->items[(deque->end + deque->capacity - 1) & (deque->capacity - 1)]; \
	}

#include "parser/operators.h"

IMPLEMENT_DEQUE(Operator);
