#include "codegen/builtin.h"

#include "parser/AST.h"
#include "codegen/llvm.h"
#include "tables/interner.h"
#include "tables/registry_manager.h"

#define BUILTIN_LIST_MEMBER(ENUM, STR) { .str = STR, .name_id = INVALID_ID, .builtin = ENUM },
struct builtin builtin_list[] = {
	BUILTIN_FOR_EACH(BUILTIN_LIST_MEMBER)
};

#define BUILTIN_LIST_COUNT (sizeof(builtin_list) / sizeof(builtin_list[0]))

#define BUILTIN_INTERN(ENUM, STR) builtin_list[ENUM].name_id = interner_intern(source_span_init(builtin_list[ENUM].str, sizeof(STR) - 1));
void builtin_intern() {
	BUILTIN_FOR_EACH(BUILTIN_INTERN);
}

char builtin_interner_id_is_inbounds(ID id) {
	return builtin_list[0].name_id.id <= id.id && id.id <= builtin_list[BUILTIN_LIST_COUNT - 1].name_id.id;
}

struct builtin builtin_get_by_intern_id(ID id) {
	ASSERT1(builtin_interner_id_is_inbounds(id));
	return builtin_list[id.id - builtin_list[0].name_id.id];
}

void gen_builtin_llvm_op(ID node_id) { 
	// a_expression expr = LOOKUP(node_id, a_expression);

	// const char * op = ((struct AST *) list_at(expr.children, 0))->value.literal.value;
	// const char * arr[3] = {0};

	// for (int i = 1; i < expr.children->size; ++i) {
	// 	arr[i - 1] = gen_expr_node(list_at(expr.children, i), self_type);
	// }

	// gen_write(format("%{u} = {s} {s} {s}, {s}\n", gen_new_register(), op, arr[0], arr[1], arr[2]));
}

void gen_builtin_llvm_store(ID node_id) {
	a_expression expr = LOOKUP(node_id, a_expression);

	if (expr.children.size != 3) {
		FATAL("Invalid builtin 'llvm_store': must only take a type, LHS and RHS value");
	}

	// const char * arr[3] = {0};

	// for (int i = 0; i < expr.children->size; ++i) {
	// 	arr[i] = gen_expr_node(list_at(expr.children, i), self_type);
	// }

	// gen_write(format("store {s} {s}, ptr {s}\n", arr[0], arr[2], arr[1]));
}

const char * gen_builtin_llvm_register_of(ID node_id) {
	a_expression expr = LOOKUP(node_id, a_expression);

	return format("%{u}", llvm_get_register_of(ARENA_GET(expr.children, 0, ID)));
}

void gen_builtin_llvm_load(ID node_id) {
	a_expression expr = LOOKUP(node_id, a_expression);

	if (expr.children.size != 1) {
		FATAL("Invalid builtin 'llvm_load': requires a variable as argument");
	}

	// struct AST * node = arena_get_ref(expr.children, 0);

	// gen_write(format("%{u} = load {s}, ptr %{u}; #llvm_load\n",  gen_new_register(), 
	// 				 llvm_type_to_llvm_type(*node->value.variable.type, self_type),
	// 				 llvm_get_register_of(node)));
}

const char * gen_builtin_llvm_type_of(ID node_id) {
	// a_expression expr = LOOKUP(node_id, a_expression);

	// ID type = ast_get_type_of(ARENA_GET(expr.children, 0, ID));

	// return llvm_type_to_llvm_type(type, self_type);
	return "";
}

const char * gen_builtin(ID node_id) {
	// a_operator op = LOOKUP(node_id, a_operator);
	// const char * builtin = op.left->value.variable.name;

	// if (!strcmp("#llvm_op", builtin)) 
	// 	gen_builtin_llvm_op(op.right, self_type);
	// else if (!strcmp("#llvm_store", builtin))
	// 	gen_builtin_llvm_store(op.right, self_type);
	// else if (!strcmp("#llvm_load", builtin))
	// 	gen_builtin_llvm_load(op.right, self_type);
	// else if (!strcmp("#llvm_register_of", builtin))
	// 	return gen_builtin_llvm_register_of(op.right, self_type);
	// else if (!strcmp("#llvm_type_of", builtin))
	// 	return gen_builtin_llvm_type_of(op.right, self_type);
	// else {
	// 	FATAL("Unknown builtin '{s}'", builtin);
	// }

	return NULL;
}
