#include <memory.h>

#include "codegen/AST.h"
#include "common/list.h"

struct List * init_list(size_t item_size) {

	struct List * list = calloc(1, sizeof(struct List));
	list->size = 0;
	list->capacity = 0;
	list->item_size = item_size;
	
	return list;
}

void free_list(struct List * list, char free_items) { 
    if (free_items) {
        for(size_t i = 0; i < list->size; ++i) {
            free(list->items[i]);
        }
    }
	free(list->items);
	free(list);
}

void list_push(struct List * list, void * item) {
	if (!list->size++) { // Incrementing here as it is just easier and not wasting a line for it
		list->items = calloc(1, list->item_size);
		list->capacity = 1;
	}

	if(list->size > list->capacity){
		list->capacity <<= 1;
		list->items = realloc(list->items, list->capacity * list->item_size);
	}

	list->items[list->size - 1] = item;
}

void list_pop(struct List * list) {
	if(!list->size) 
		return;
    
	list->items[--list->size] = NULL;
}

void list_shrink(struct List * list, unsigned int new_size) {	
	if (list->size == 0)
		return;

	while (list->size != new_size)
        list_pop(list);
}

void* list_at(struct List * list, int index) {
    const int size = list->size; // needs to be signed for the "anti-negative" index thing to work
    return list->items[(size + (index % size)) % size];
}

/* void print_list(struct List * list) { */
/* 	 */
/* 	println("<size={u}, capacity={u}, item_size={u}>", list->size, list->capacity, list->item_size); */
/* 	for(size_t i = 0; i < list->size; ++i) { */
/* 		struct Ast * item = list_at(list, i); */
/* 		print_ast("\t{s}\n", item); */
/* 	} */
/*  */
/* } */

void list_reserve(struct List * list, unsigned int additions) {
	
	list->capacity += additions;
	list->items = realloc(list->items, list->capacity * list->item_size);

}

struct List * list_copy(struct List * list, size_t start, size_t end) {
	struct List * copy = init_list(list->item_size);
	if (list == NULL || list->size == 0) {
		return copy;
	}

	copy->size = end - start;
	list_reserve(copy, copy->size);	

	for (size_t i = start; i < end; ++i) {
		copy->items[i] = list->items[i];
	}

	return copy;
}

struct List * list_combine(struct List * first, struct List * second) {
    if (first->item_size != second->item_size) {
        println("INVALID LIST COMBINE - sizes: first={u}, second={u}", first->item_size, second->item_size);
        exit(1);
    }

    struct List * dest = init_list(first->item_size);
    list_reserve(dest, first->size + second->size);
    dest->size = dest->capacity;

    for (int i = 0; i < first->size; ++i) {
        dest->items[i] = first->items[i];
    }

    for (int i = 0; i < second->size; ++i) {
        dest->items[i + first->size] = second->items[i];
    }

    return dest;
}
