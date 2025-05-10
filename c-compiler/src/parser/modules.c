#include "parser/modules.h"
#include "codegen/AST.h"
#include "common/hashmap.h"
#include "common/list.h"

void add_function_to_module(struct Ast * module, struct Ast * function) {
    a_module * dest = module->value;
    a_function * func = function->value;

    struct List * functions = hashmap_get(dest->functions_map, func->name);
    if (functions == NULL) {
        functions = init_list(sizeof(struct Ast *));
        hashmap_set(dest->functions_map, func->name, functions);
    }
    
    list_push(functions, function);
}

void include_module(struct Ast * dest_ast, struct Ast * src_ast) {
    a_module * dest = dest_ast->value, * src = src_ast->value;    
    hashmap_combine(dest->functions_map, src->functions_map);
}

struct Ast * find_module(struct Ast * root_ast, const char * module_name) {
    struct Ast * temp;
    a_root * root = root_ast->value;
    a_module * module;

    for (int i = 0; i < root->modules->size; ++i) {
        module = (temp = root->modules->items[i])->value;

        if (!strcmp(module_name, module->path)) {
            return temp;
        }
    }

    return NULL;
}
