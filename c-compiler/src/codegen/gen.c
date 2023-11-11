#include "codegen/gen.h"
#include "codegen/AST.h"
#include "codegen/llvm.h"
#include "parser/types.h"

FILE * output = NULL;
struct Generator generator = {0};

void gen_load(struct Ast * type, unsigned int dest, unsigned int src) {
    writef(output, "%{u} = load {s}, ptr %{u}\n", dest, llvm_ast_type_to_llvm_type(type), src);
}

void gen_expr_node(struct Ast * ast) {
    switch (ast->type) {
        case AST_OP:
        {

        } break;
        case AST_LITERAL:
        {

        } break;
        case AST_VARIABLE:
        {

        } break;
        default:
            logger_log(format("AST type '{s}' code generation is not implemented for expr node", ast_type_to_str(ast->type)), IR, ERROR);
            exit(1);
    }
}

void gen_expr(struct Ast * ast) {
    a_expr * expr = ast->value;

    for (int i = 0; i < expr->children->size; ++i) {
        print_ast("{s}\n", list_at(expr->children, i));
    }

}

void gen_scope(struct Ast * ast) {
    a_scope * scope = ast->value;
    
    for (int i = 0; i < scope->variables->size; ++i) {
        a_variable * var = ((struct Ast *) list_at(scope->variables, i))->value;
        writef(output, "%{u} = alloca {s}\n", generator.reg_count++, llvm_ast_type_to_llvm_type(var->type));
    }

    for (int i = 0; i < scope->nodes->size; ++i) {
        struct Ast * node = list_at(scope->nodes, i);
        switch (node->type) {
            case AST_EXPR:
                gen_expr(node); break;
            case AST_DECLARATION:
                gen_expr(((a_declaration *) node->value)->expression); break;
            default:
                logger_log(format("AST type '{s}' code generation is not implemented in scope", ast_type_to_str(node->type)), IR, ERROR);
                exit(1);
        }
    }

}

int gen_function_argument_list(struct Ast * ast) {
    a_expr * expr = ast->value;
 
    for (int i = 0; i < expr->children->size; ++i) {
        if (i != 0) {
            fputs(", ", output);
        }

        a_variable * var = ((struct Ast *) list_at(expr->children, i))->value;

        writef(output, "{s} noundef %{i}", llvm_ast_type_to_llvm_arg_type(
                    ((a_variable *)
                        ((struct Ast *) 
                            list_at(expr->children, i))
                        ->value)
                    ->type), generator.reg_count++);
    }

    fputs(") {\n", output);

    return expr->children->size;
}

void gen_function(struct Ast * ast) {
    a_function * func = ast->value;

    if (func->is_inline)
        return;

    generator.reg_count = 0, generator.block_count = 0;

    const char * return_type_str = llvm_ast_type_to_llvm_type(func->return_type);
    
    writef(output, "\ndefine dso_local {s} @{s}(", return_type_str, func->name); 
    gen_function_argument_list(func->arguments);

    generator.ret_reg = ++generator.reg_count;
    generator.reg_count++;
    writef(output, "%{u} = alloca {s}\n", generator.ret_reg, return_type_str);

    gen_scope(func->body);
    
    writef(output, "exit:\n%{u} = load {s}, ptr %{u}\nret {s} %{u}\n", 
            generator.reg_count, return_type_str, generator.ret_reg, return_type_str, generator.reg_count);

    fputs("}\n", output);

}

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

    for (int i = 0; i < _struct->functions->size; ++i) {
        struct Ast * node = list_at(_struct->functions, i);
        a_function * func = node->value;
        ASSERT1(func->name != NULL);
        func->name = format("{s}_{s}", _struct->name, func->name);
        println("name: {s}", func->name);
        gen_function(node);
    }

}

void gen_module(struct Ast * ast) {
    a_module * module = ast->value;
    
    for (int i = 0; i < module->structures->size; ++i) {
        gen_structs(list_at(module->structures, i));
    }

    for (int i = 0; i < module->variables->size; ++i) {
        print_ast(format("{u!} global: {s}\n"), list_at(module->variables, i));
    }
     
    for (int i = 0; i < module->functions->size; ++i) {
        gen_function(list_at(module->functions, i));
    }

}

void gen(FILE * fp, struct Ast * ast) {
    ASSERT1(ast->type == AST_ROOT);
    ASSERT1(fp != NULL);
    a_root * root = ast->value;
    output = fp;

    for (int i = 0; i < root->modules->size; ++i) {
        gen_module(list_at(root->modules, i));
    }
}
