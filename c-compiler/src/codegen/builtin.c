#include "codegen/builtin.h"
#include "codegen/llvm.h"

void gen_builtin_llvm_op(struct Ast * ast, struct Ast * self_type) { 
    a_expr * expr = ast->value;
    
    const char * op = ((a_literal*) ((struct Ast *) list_at(expr->children, 0))->value)->value;
    const char * arr[3] = {0};

    for (int i = 1; i < expr->children->size; ++i) {
        arr[i - 1] = gen_expr_node(list_at(expr->children, i), self_type);
    }

    gen_write(format("%{u} = {s} {s} {s}, {s}", gen_new_register(), op, arr[0], arr[1], arr[2]));
}

void gen_builtin_llvm_store(struct Ast * ast, struct Ast * self_type) {
    struct Ast * node;
    a_expr * expr = ast->value; 

    if (expr->children->size != 3) {
        logger_log("Invalid builtin 'llvm_store': must only take a type, LHS and RHS value", IR, ERROR);
        exit(1);
    }

    const char * arr[3] = {0};
    
    for (int i = 0; i < expr->children->size; ++i) {
        arr[i] = gen_expr_node(list_at(expr->children, i), self_type);
    }

    gen_write(format("store {s} {s}, ptr {s}\n", arr[0], arr[2], arr[1]));
}

const char * gen_builtin_llvm_register_of(struct Ast * ast, struct Ast * self_type) {
    a_expr * expr = ast->value;

    return format("%{u}", llvm_get_register_of(list_at(expr->children, 0)));
}

void gen_builtin_llvm_load(struct Ast * ast, struct Ast * self_type) {
    a_expr * expr = ast->value;

    if (expr->children->size != 1) {
        logger_log("Invalid builtin 'llvm_load': requires a variable as argument", IR, ERROR);
        exit(1);
    }

    struct Ast * node = list_at(expr->children, 0);

    gen_write(format("%{u} = load {s}, ptr %{u}; #llvm_load\n",  gen_new_register(), 
                                                llvm_ast_type_to_llvm_type(((a_variable *) node->value)->type, self_type),
                                                llvm_get_register_of(node)));
}

const char * gen_builtin_llvm_type_of(struct Ast * ast, struct Ast * self_type) {
    a_expr * expr = ast->value;
    
    struct Ast * type = ast_get_type_of(list_at(expr->children, 0));

    return llvm_ast_type_to_llvm_type(type, self_type);
}

const char * gen_builtin(struct Ast * ast, struct Ast * self_type) {
    a_op * op = ast->value;
    const char * builtin = ((a_variable *) op->left->value)->name;
    
    if (!strcmp("#llvm_op", builtin)) 
        gen_builtin_llvm_op(op->right, self_type);
    else if (!strcmp("#llvm_store", builtin))
        gen_builtin_llvm_store(op->right, self_type);
    else if (!strcmp("#llvm_load", builtin))
        gen_builtin_llvm_load(op->right, self_type);
    else if (!strcmp("#llvm_register_of", builtin))
        return gen_builtin_llvm_register_of(op->right, self_type);
    else if (!strcmp("#llvm_type_of", builtin))
        return gen_builtin_llvm_type_of(op->right, self_type);
    else {
        logger_log(format("Unknown builtin '{s}'", builtin), IR, ERROR);
        exit(1);
    }

    return NULL;
}
