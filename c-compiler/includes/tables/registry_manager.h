#pragma once

#include "common/ID.h"
#include "tables/registry.h"

#include "tables/interner.h"
#include "parser/types.h"
#include "parser/AST.h"
#include "checker/typing/typechecker.h"

#define AST_REGISTRY_MANAGER_DECL_EL(ENUM, STR, NAME, ...) Registry NAME;
struct registry_manager {
	Registry type;
    REGISTRY_KINDS(AST_REGISTRY_MANAGER_DECL_EL)
};

#define AST_REGISTRY_MANAGER_INIT_REGISTRY(ENUM, STR, TYPE, ...) manager.TYPE = registry_init(ENUM, sizeof(TYPE));
static struct registry_manager registry_manager_init() {
	struct registry_manager manager;
    REGISTRY_KINDS(AST_REGISTRY_MANAGER_INIT_REGISTRY)
	return manager;
}

static inline void type_init_intrinsic_type(enum id_type type, void * type_ref) {
    switch (type) {
        case ID_TUPLE_TYPE:
            ((Tuple_T *) type_ref)->types = arena_init(sizeof(ID));
            break;
        case ID_SYMBOL_TYPE:
            ((Symbol_T *) type_ref)->templates = arena_init(sizeof(ID));
            break;
        case ID_PLACE_TYPE:
        case ID_FN_TYPE:
        case ID_NUMERIC_TYPE:
        case ID_ARRAY_TYPE:
        case ID_REF_TYPE:
            break;
        default:
            FATAL("Invalid ID type: {s}", id_type_to_string(type));
    }
}

#define FILL_INTERNER_ID(REF, INTERNER_ID, ...) (((struct interner_entry *) REF)->id = INTERNER_ID);
#define FILL_SYMBOL_ID(REF, SYMBOL_ID, ...) (((struct symbol_map_entry *) REF)->symbol_id = SYMBOL_ID);
#define FILL_TC_ID(REF, TC_ID, ...) tc_node_init(TC_ID, REF);
#define FILL_TYPE_ID(REF, TYPE_ID, ...) (((struct type_info *) REF)->type_id = TYPE_ID); type_init_intrinsic_type(TYPE_ID.type, REF);
#define FILL_AST_ID(REF, NODE_ID, SCOPE_ID) (((struct AST_info *) REF)->node_id = NODE_ID, ((struct AST_info *) REF)->scope_id = SCOPE_ID); ast_init_node(NODE_ID.type, REF);

#define AST_REGISTRY_MANAGER_REGISTRY_ALLOCATE(ENUM, STR, TYPE, KIND) \
    case ENUM: ptr = registry_allocate(&manager->TYPE, &node_id); FILL_##KIND##_ID(ptr, node_id, scope_id); break;
static inline void * registry_manager_allocate(struct registry_manager * manager, enum id_type type, ID scope_id) {
    ID node_id;
    void * ptr;

    switch (type) {
        REGISTRY_KINDS(AST_REGISTRY_MANAGER_REGISTRY_ALLOCATE)
        default:
            ERROR("Invalid id_type");
            exit(1);
    }

    return ptr;
}

#define AST_REGISTRY_MANAGER_REGISTRY_LOOKUP(ENUM, STR, TYPE, ...) case ENUM: return registry_lookup(manager->TYPE, node_id);
static inline void * registry_manager_lookup(struct registry_manager * manager, ID node_id) {
    switch (node_id.type) {
        REGISTRY_KINDS(AST_REGISTRY_MANAGER_REGISTRY_LOOKUP)
        default:
            FATAL("Invalid id_type: {s}", id_type_to_string(node_id.type));
    }
}

#define AST_REGISTRY_MANAGER_REGISTRY_REMOVE(ENUM, STR, TYPE, ...) case ENUM: registry_remove(&manager->TYPE, node_id); break;
static inline void registry_manager_remove(struct registry_manager * manager, ID node_id) {
    switch (node_id.type) {
        REGISTRY_KINDS(AST_REGISTRY_MANAGER_REGISTRY_REMOVE)
        default:
            FATAL("Invalid id_type");
    }
}

#define AST_REGISTRY_MANAGER_REGISTRY_FREE(ENUM, STR, TYPE, ...) registry_free(&(manager->TYPE));
static inline void registry_manager_free(struct registry_manager * manager) {
    REGISTRY_KINDS(AST_REGISTRY_MANAGER_REGISTRY_FREE);
}

#define LOOP_OVER_REGISTRY(TYPE, VAR, CODE) { \
	Registry _REGISTRY = registry_manager_get().TYPE; \
	for (size_t _I = 0, _BLOCK = 0; _BLOCK < _REGISTRY.entries.block_count; ++_BLOCK) { \
		for (size_t _BI = 0; _BI < _REGISTRY.entries.block_max_item_count && _I < _REGISTRY.entries.item_count; ++_BI, ++_I) { \
			TYPE * VAR = block_arena_get_ref(_REGISTRY.entries, _I); \
			CODE \
		} \
	} \
}

void registry_manager_setup_instance();
const struct registry_manager registry_manager_get();

struct symbol_map_entry * symbol_allocate();
struct interner_entry * interner_allocate();
void * tc_allocate(enum id_type type);
void * ast_allocate(enum id_type type, ID scope_id);
void * type_allocate(enum id_type type);

#define LOOKUP(node_id, type) (*(type *) lookup(node_id))

struct symbol_table_entry * symbol_lookup(ID symbol_id);
struct interner_entry * interner_lookup(ID interner_id);
void * lookup(ID ast_id);

void _symbol_remove_only_last_allowed(ID symbol_id);
