#pragma once

#include "common/common.h"
#include "common/io.h"
#include "codegen/AST.h"
#include "codegen/checker.h"
#include "codegen/AST.h"
#include "codegen/builtin.h"
#include "codegen/llvm.h"
#include "parser/types.h"

struct Generator {
    unsigned int reg_count;
    unsigned int block_count;
    unsigned int ret_reg;
    char * ret_type;
};

// void gen_write(const char * src);
//
// void gen_load(const char * type, unsigned int dest, unsigned int src);
// unsigned int gen_new_register();
//
// const char * gen_expr_node(struct AST * ast, struct AST * self_type);
// void gen_expr(struct AST * ast, struct AST * self_type);
//
// void gen_scope(struct AST * ast, struct AST * self_type);
// void gen_function_argument_list(struct AST * ast, struct AST * self_type);
//
// void gen_function_with_name(struct AST * ast, const char * name, struct AST * self_type);
// void gen_function(struct AST * ast, struct AST * self_type);
// void gen_inline_function(struct AST * ast, struct List * arguments, struct AST * self_type);
//
// void gen_structs(struct AST * ast, struct AST * self_type);
// void gen_impl(struct AST * ast);
// void gen_module(struct AST * ast);
//
// void gen(FILE * fp, struct AST * ast);
