#pragma once

#include "codegen/AST.h"
#include "common/hashmap.h"

struct Checker {
    struct HashMap * variables;
    struct HashMap * functions;
    // struct HashMap * types;
    // struct HashMap * traits;
    // struct HashMap * implementations;
};

void checker_check_declaration(struct Checker *, struct Ast *);
void checker_check_function(struct Checker *, struct Ast *);

void checker_check_module(struct Checker *, struct Ast *);

struct Checker * checker_check(struct Ast *);
