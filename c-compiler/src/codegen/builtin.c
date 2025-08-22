#include "codegen/builtin.h"
#include "codegen/llvm.h"
#include "common/logger.h"

#define BUILTIN_LIST_MEMBER(ENUM, STR) { .str = STR, .name_id = INVALID_INTERN_ID, .builtin = ENUM },
struct builtin builtin_list[] = {
	BUILTIN_FOR_EACH(BUILTIN_LIST_MEMBER)
};

#define BUILTIN_LIST_COUNT (sizeof(builtin_list) / sizeof(builtin_list[0]))

#define BUILTIN_INTERN(ENUM, STR) builtin_list[ENUM].name_id = interner_intern(STRING_FROM_LITERAL(STR));

void builtin_intern() {
	BUILTIN_FOR_EACH(BUILTIN_INTERN);
}

char builtin_interner_id_is_inbounds(unsigned int ID) {
	return builtin_list[0].name_id <= ID && ID <= builtin_list[BUILTIN_LIST_COUNT - 1].name_id;
}

struct builtin builtin_get_by_intern_id(unsigned int ID) {
	ASSERT1(builtin_interner_id_is_inbounds(ID));
	return builtin_list[ID - builtin_list[0].name_id];
}

void gen_builtin_llvm_op(struct AST * ast, struct AST * self_type) { 
	a_expression expr = ast->value.expression;

	// const char * op = ((struct AST *) list_at(expr.children, 0))->value.literal.value;
	// const char * arr[3] = {0};

	// for (int i = 1; i < expr.children->size; ++i) {
	// 	arr[i - 1] = gen_expr_node(list_at(expr.children, i), self_type);
	// }

	// gen_write(format("%{u} = {s} {s} {s}, {s}\n", gen_new_register(), op, arr[0], arr[1], arr[2]));
}

void gen_builtin_llvm_store(struct AST * ast, struct AST * self_type) {
	struct AST * node;
	a_expression expr = ast->value.expression;

	if (expr.children.size != 3) {
		FATAL("Invalid builtin 'llvm_store': must only take a type, LHS and RHS value");
	}

	const char * arr[3] = {0};

	// for (int i = 0; i < expr.children->size; ++i) {
	// 	arr[i] = gen_expr_node(list_at(expr.children, i), self_type);
	// }

	// gen_write(format("store {s} {s}, ptr {s}\n", arr[0], arr[2], arr[1]));
}

const char * gen_builtin_llvm_register_of(struct AST * ast) {
	a_expression expr = ast->value.expression;

	return format("%{u}", llvm_get_register_of(arena_get_ref(expr.children, 0)));
}

void gen_builtin_llvm_load(struct AST * ast) {
	a_expression expr = ast->value.expression;

	if (expr.children.size != 1) {
		FATAL("Invalid builtin 'llvm_load': requires a variable as argument");
	}

	struct AST * node = arena_get_ref(expr.children, 0);

	// gen_write(format("%{u} = load {s}, ptr %{u}; #llvm_load\n",  gen_new_register(), 
	// 				 llvm_type_to_llvm_type(*node->value.variable.type, self_type),
	// 				 llvm_get_register_of(node)));
}

const char * gen_builtin_llvm_type_of(struct AST * ast, struct AST * self_type) {
	a_expression expr = ast->value.expression;

	Type type = ast_get_type_of(arena_get_ref(expr.children, 0));

	return llvm_type_to_llvm_type(type, self_type);
}

const char * gen_builtin(struct AST * ast, struct AST * self_type) {
	a_operator op = ast->value.operator;
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
