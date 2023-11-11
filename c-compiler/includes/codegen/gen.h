#pragma once

#include "common/common.h"
#include "common/io.h"
#include "codegen/AST.h"
#include "codegen/checker.h"

struct Generator {
    unsigned int reg_count;
    unsigned int block_count;
    unsigned int ret_reg;
};

void gen(FILE * fp, struct Ast * root);
