#pragma once

#include "parser/modules.h"
#include "codegen/AST.h"
#include "common/hashmap.h"
#include "common/logger.h"

struct AST * add_module(struct Parser * parser, char * path) {
	khash_t(modules_hm) * hashmap = &parser->root->value.root.modules;

	int retcode;
	khint_t k = kh_put(modules_hm, hashmap, path, &retcode);
	ASSERT1(k != kh_end(hashmap));

	if (retcode == KEY_ALREADY_PRESENT) {
		return kh_value(hashmap, k);
	}

	struct AST * module = init_ast(AST_MODULE, parser->root);
	module->value.module.file_path = path;

	kh_value(hashmap, k) = module;
	ARENA_APPEND(&parser->modules_to_parse, path);

	return module;
}

struct AST * find_module(struct AST * root, const char * module_path) {
	khash_t(modules_hm) * hashmap = &root->value.root.modules;
	khint_t k = kh_get(modules_hm, hashmap, module_path);

	if (k == kh_end(hashmap)) {
		return NULL;
	}

	return kh_value(hashmap, k);
}

struct AST * module_lookup_symbol(struct AST * symbol_ast) {
	struct AST * module_ast = get_scope(AST_MODULE, symbol_ast->scope);
}
