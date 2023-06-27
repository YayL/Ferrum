#include "list.h"

#include <memory.h>

#include "AST.h"
#include "list.h"


struct List * init_list(size_t item_size) {

	struct List * list = calloc(1, sizeof(struct List));
	list->size = 0;
	list->capacity = 0;
	list->item_size = item_size;
	
	return list;
}

void free_list(struct List * list) {

	for(size_t i = 0; i < list->size; ++i) {
		memset(list->items[i], 0, list->item_size);
		free(list->items[i]);
	}
	free(list->items);
	free(list);

}

void list_push(struct List * list, void* item) {
	if (!list->size++) { // Incrementing here as it is just easier and not wasting a line for it
		list->items = calloc(1, list->item_size);
		list->capacity = 1;
	}

	if(list->size > list->capacity){
		list->items = realloc(list->items, list->size * list->item_size);
		list->capacity = list->size;
	}

	list->items[list->size - 1] = item;
}

void list_pop(struct List * list) {
	if(!list->size) 
		return;

	list->items[list->size] = NULL;
	free(list->items[list->size--]);
}

void list_shrink(struct List * list, unsigned int new_size) {	
	if (list->size == 0)
		return;

	while (list->size != new_size) list_pop(list);

}

void* list_at(struct List * list, int index) {

	if(0 <= index && index < list->size)
		return list->items[index];

	return NULL;

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

void * list_copy(struct List * list, size_t start, size_t end) {
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
