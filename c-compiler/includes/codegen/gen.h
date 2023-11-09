#pragma once

#include "common/common.h"
#include "common/io.h"
#include "codegen/AST.h"
#include "codegen/checker.h"

struct Generator {
    String * globals;
    String * current;
    int reg_count;
    int block_count;
};

void gen(FILE * fp, struct Ast * root);
