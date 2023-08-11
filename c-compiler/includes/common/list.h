#pragma once

#include "common/common.h"

struct List {
	void** items;
	size_t size;
	size_t capacity;
	size_t item_size;
};

/**
 * @brief Initialize a new list with size n
 * 
 * @param n
 * @return struct List* 
 */

struct List * init_list(size_t);


/**
 * @brief Free an initialized list
 * 
 */
void free_list(struct List *, char free_items);

/**
 * @brief Push a new item to the top of the list
 * 
 * @param list 
 * @param item
 */
void list_push(struct List *, void *);

/**
 * @brief Remove the last element in the list from memory but preserve capacity.
 * 
 * @param list 
 * @return void* 
 */

void list_pop(struct List *);

/**
 * @brief Resize the array by popping multiple values
*/

void list_shrink(struct List *, unsigned int);

/**
 * @brief Return the item at a certain index in the list
 * 
 * @param list 
 * @param index 
 * @return void* 
 */
void* list_at(struct List *, int);

/**
 * @brief Print a list with a neat format
 * 
 */
void print_list(struct List *);

/**
 * @brief Pre allocate multiple slots for future use
*/
void list_reserve(struct List *, unsigned int);

/**
 * @brief Copy the current list
 * @return Pointer to the list cop copy
 */
void * list_copy(struct List * src, size_t start, size_t end);
