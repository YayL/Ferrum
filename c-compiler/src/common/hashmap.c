#include "common/hashmap.h"

#include "fmt.h"

struct HashMap * new_HashMap(size_t pow_capacity) {

	struct HashMap * map = malloc(sizeof(struct HashMap));
	
	map->capacity = (1 << pow_capacity) - 1;
	map->buckets = 0;
	map->total = 0;

	map->bucket_list = calloc(map->capacity + 1, sizeof(HM_Pair *));

	return map;
}

long HM_HashCode(struct HashMap * map, const char * key) {

	const int p = (2 << 7) - 1;
	const int m = (2 << 24) - 1;
	
	long long hash_value = 0;
	long long p_pow = 31;

	for (int i = 0; key[i]; ++i) {
		hash_value = (((hash_value + (key[i] - ' ' + 1)) * p_pow) & m);
		p_pow = (p_pow * p) & m;
	}
	
	return hash_value & map->capacity;
}

void * HM_get(struct HashMap * map, const char * key) {
	HM_Pair * current = map->bucket_list[HM_HashCode(map, key)];

	while (current) {
		if (!strcmp(current->key, key))
			return current->value;
		current = current->next;
	}
	
	return NULL;
	println("[HashMap]: Key '{s}' was not found", key);
	exit(1);
}

long HM_has(struct HashMap * map, const char * key) {
	HM_Pair * current = map->bucket_list[HM_HashCode(map, key)];

	while (current) {
		if (!strcmp(current->key, key))
			return 1;
		current = current->next;
	}

	return 0;
}

void HM_set(struct HashMap * map, char * key, void* value) {

	long long index = HM_HashCode(map, key);
	HM_Pair * current = map->bucket_list[index];

	while (current) {
		if (!strcmp(current->key, key)) {
			current->value = value;
			return;
		}
		current = current->next;
	}

    HM_Pair * p = malloc(sizeof(HM_Pair));
    p->key = key;
    p->value = value;
    p->next = map->bucket_list[index];
	if (p->next == NULL)
		++map->buckets;
    map->bucket_list[index] = p;
    map->total++;
}

void * HM_remove(struct HashMap * map, const char * key) {
	
	long long index = HM_HashCode(map, key);
	HM_Pair * current = map->bucket_list[index], * temp;
	size_t depth = 0;

	while (current) {
		if (!strcmp(current->key, key))
			break;
		++depth;
		current = current->next;
	}
	
	if (current == NULL) {
		println("[HashMap]: Key '{s}' was not found", key);
		exit(1);
	}

	if (depth == 0) {
		map->bucket_list[index] = current->next;
	} else {
		temp = map->bucket_list[index];	
		for (int i = 0; i < depth - 1; ++i)
			temp = temp->next;
		temp->next = current->next;	
	}
	if (map->bucket_list[index] == NULL)
		--map->buckets;
	--map->total;


	void * value = current->value;
	
	free(current->key);
	free(current);
	
	return value;
}

void HM_print(struct HashMap * map) {
	HM_Pair * bucket, * current;
	println("[Buckets: {lu}, Total: {lu}]:", map->buckets, map->total);
	for (int i = 0; i < map->capacity; ++i) {
		bucket = map->bucket_list[i];
		current = bucket;
		while (current) {
			println("\t{s}: {Li}", current->key, current->value);
			current = current->next;
		}
	}
}

void HM_clear (struct HashMap * map) {

	HM_Pair * current, * next;
	for (int i = 0; i < map->capacity; ++i) {
		current = map->bucket_list[i];
		while (current) {
			next = current->next;
			free(current->key);
			free(current);
			current = next;
		}
		map->bucket_list[i] = NULL;
	}
}

void HM_free(struct HashMap * map) {
	
	HM_Pair * current, * next;

	for (int i = 0; i < map->capacity; ++i) {
		current = map->bucket_list[i];
		while (current) {
			next = current->next;
			free(current->key);
			free(current);
			current = next;
		}
	}
	
	free(map->bucket_list);
	free(map);
}
