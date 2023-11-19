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
};

void gen_write(const char * src);

void gen_load(const char * type, unsigned int dest, unsigned int src);
unsigned int gen_new_register();

const char * gen_expr_node(struct Ast * ast, struct Ast * self_type);
void gen_expr(struct Ast * ast, struct Ast * self_type);

void gen_scope(struct Ast * ast, struct Ast * self_type);
void gen_function_argument_list(struct Ast * ast, struct Ast * self_type);

void gen_function_with_name(struct Ast * ast, const char * name, struct Ast * self_type);
void gen_function(struct Ast * ast, struct Ast * self_type);
void gen_inline_function(struct Ast * ast, struct List * arguments, struct Ast * self_type);

void gen_structs(struct Ast * ast, struct Ast * self_type);
void gen_impl(struct Ast * ast);
void gen_module(struct Ast * ast);

void gen(FILE * fp, struct Ast * ast);
