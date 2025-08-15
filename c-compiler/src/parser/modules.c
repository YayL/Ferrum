#include "parser/modules.h"
#include "codegen/AST.h"
#include "common/hashmap.h"
#include "common/list.h"

void add_function_to_module(struct AST * module, struct AST * function) {
    a_module dest = module->value.module;
    a_function func = function->value.function;

    // struct List * functions = hashmap_get(dest.functions_map, func.name);
    // if (functions == NULL) {
    //     functions = init_list(sizeof(struct AST *));
    //     hashmap_set(dest.functions_map, func.name, functions);
    // }
    
    // list_push(functions, function);
}

void include_module(struct AST * dest_ast, struct AST * src_ast) {
    a_module dest = dest_ast->value.module, src = src_ast->value.module;
    // hashmap_combine(dest.functions_map, src.functions_map);
}

struct AST * find_module(struct AST * root_ast, const char * module_name) {
    struct AST * temp;
    a_root root = root_ast->value.root;
    a_module * module;

    for (int i = 0; i < root.modules->size; ++i) {
        module = &(temp = root.modules->items[i])->value.module;

        if (!strcmp(module_name, module->path)) {
            return temp;
        }
    }

    return NULL;
}
