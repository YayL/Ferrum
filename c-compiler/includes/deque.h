#pragma once

#include "common.h"

struct Deque {
	void** items;
	int start;
	int end;
	int size;
	size_t capacity;
	size_t item_size;
};

struct Deque * init_deque (size_t item_size);

size_t deque_find (struct Deque * deque, void * comp);

void deque_expand (struct Deque * deque);

void push_back (struct Deque * deque, void * item);

void push_front (struct Deque * deque, void * item);

void pop_back (struct Deque * deque);

void pop_front (struct Deque * deque);

void * deque_front (struct Deque * deque);

void * deque_back (struct Deque * deque);

void deque_rotate (struct Deque * deque, long long rotate);

void * deque_index (struct Deque * deque, size_t index);

void * deque_remove (struct Deque * deque, int index);

struct Deque * deque_copy (struct Deque * src);

void deque_print (struct Deque * deque, void (*print_item)());
