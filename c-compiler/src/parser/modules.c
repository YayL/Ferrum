#include "parser/modules.h"

#include "parser/AST.h"
#include "common/data/hashmap.h"
#include "common/logger.h"
#include "tables/registry_manager.h"

ID add_module(struct Parser * parser, char * path) {
	khash_t(map_string_to_id) * hashmap = &parser->root->modules;

	int retcode;
	khint_t k = kh_put(map_string_to_id, hashmap, path, &retcode);
	ASSERT1(k != kh_end(hashmap));

	if (retcode == KH_PUT_ALREADY_PRESENT) {
		return kh_value(hashmap, k);
	}

	a_module * module = ast_allocate(ID_AST_MODULE, parser->root->info.node_id);
	module->file_path = path;

	kh_value(hashmap, k) = module->info.node_id;
	ARENA_APPEND(&parser->modules_to_parse, path);

	return module->info.node_id;
}

ID find_module(a_root * root, const char * module_path) {
	khash_t(map_string_to_id) * hashmap = &root->modules;
	khint_t k = kh_get(map_string_to_id, hashmap, module_path);

	if (k == kh_end(hashmap)) {
		return INVALID_ID;
	}

	return kh_value(hashmap, k);
}
