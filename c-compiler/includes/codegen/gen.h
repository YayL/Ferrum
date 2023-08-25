#pragma once

#include "common/common.h"
#include "common/io.h"
#include "codegen/AST.h"
#include "codegen/checker.h"


// TODO:
//
// Visitor:
// Go through the AST to find variables and different types of references
// to defined values. If a value is not defined then error
//
// Type checker:
// Add the specified type to all variables
// Go through expressions and check that the types are correct
//
// LLVM generator:
// Go through the AST and turn it into LLVM IR

// The job of the generator is to 
struct Generator {
    String * globals;
    String * current;
    int reg_count;
    int block_count;
};

struct Generator * init_generator();

struct Ast * get_function(struct Ast * scope, char * name);

void gen_allocate_variable(String * str, struct Generator * gen, struct Ast * ast);
void gen_global_variable(struct Generator * gen, struct Ast * ast);

void gen_call(struct Generator * gen, struct Ast * ast);
void gen_op(struct Generator * gen, struct Ast * ast);

void gen_if(struct Generator * gen, struct Ast * ast);
void gen_while(struct Generator * gen, struct Ast * ast);
void gen_do(struct Generator * gen, struct Ast * ast);

void gen_expr(struct Generator * gen, struct Ast * ast);

void gen_scope(struct Generator * gen, struct Ast * ast);
void gen_function(struct Generator * gen, struct Ast * ast);

void gen_module(struct Generator * gen, struct Ast * ast);

void gen(struct Ast * ast);
