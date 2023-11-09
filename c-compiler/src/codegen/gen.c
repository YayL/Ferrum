#include "codegen/gen.h"
#include "codegen/llvm.h"

FILE * output = NULL;

void gen_structs(struct Ast * ast) {
    a_struct * _struct = ast->value;

    writef(output, "%struct.{s} = type {c}", _struct->name, '{');

    for (int i = 0; i < _struct->variables->size; ++i) {
        struct Ast * node = list_at(_struct->variables, i);
        if (i != 0)
            fputc(',', output);
        writef(output, " {s}", llvm_ast_type_to_llvm_type(((a_variable *) node->value)->type));
    }

    writef(output, " }\n");
}

void gen_module(struct Ast * ast) {
    a_module * module = ast->value;
    
    for (int i = 0; i < module->structures->size; ++i) {
        gen_structs(list_at(module->structures, i));
    }

}

void gen(FILE * fp, struct Ast * ast) {
    ASSERT1(ast->type == AST_ROOT);
    ASSERT1(fp != NULL);
    output = fp;

    a_root * root = ast->value;

    for (int i = 0; i < root->modules->size; ++i) {
        gen_module(list_at(root->modules, i));
    }
}
