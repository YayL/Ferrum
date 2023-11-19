#include "codegen/gen.h"
#include "codegen/AST.h"
#include "codegen/llvm.h"
#include "parser/types.h"

FILE * output = NULL;
struct Generator generator = {0};

unsigned int gen_new_register() {
    return generator.reg_count++;
}

void gen_write(const char * src) {
    fputs(src, output);
}

void gen_call_with_info(const char * name, const char * ret_type, struct List * arg_types, struct List * arg_values, struct Ast * self_type) {
    writef(output, "%{u} = call {s} @{s}(", generator.reg_count++, ret_type, name);

    for (int i = 0; i < arg_types->size; ++i) {
        if (i != 0)
            fputs(", ", output);
        struct Ast * type = list_at(arg_types, i);
        writef(output, "{s} noundef {s}", llvm_ast_type_to_llvm_arg_type(list_at(arg_types, i), self_type), list_at(arg_values, i));
    }

    fputs(")\n", output);
}

const char * gen_call(struct Ast * ast, struct Ast * self_type) {
    a_op * op = ast->value;

    if (((a_variable *) op->left->value)->name[0] == '#') {
        return gen_builtin(ast, self_type);
    }

    a_expr * args = op->right->value;
    a_function * func = op->definition->value;

    struct List * parameters = init_list(sizeof(char *));

    const unsigned int size = args->children->size;

    for (int i = 0; i < size; ++i) {
        list_push(parameters, (void *) gen_expr_node(list_at(args->children, i), self_type));
    }
    
    gen_call_with_info(func->name, llvm_ast_type_to_llvm_type(op->type, self_type), ast_to_ast_type_list(func->param_type), parameters, self_type);
    return NULL;
}

void gen_inline_function(struct Ast * ast, struct List * arguments, struct Ast * self_type) {
    a_function * func = ast->value;

    struct List * params = ast_to_ast_type_list(func->param_type);

    struct List * list = init_list(sizeof(char *));

    for (int i = 0; i < arguments->size; ++i) {
        list_push(list, (void *) gen_expr_node(list_at(arguments, i), self_type));
    }

    writef(output, "\n; inline call: {s}\n", func->name);

    for (int i = 0; i < arguments->size; ++i) {
        const char * type_str = llvm_ast_type_to_llvm_type(list_at(params, i), self_type);
        struct Ast * param = list_at(((a_expr *) func->arguments->value)->children, i),
                   * arg = list_at(arguments, i);
    
        if (arg->type == AST_VARIABLE) {
            ((a_variable *) param->value)->reg = ((a_variable *) arg->value)->reg;
        } else {
            ((a_variable *) param->value)->reg = generator.reg_count;

            writef(output, "%{u} = alloca {s}\n", generator.reg_count, type_str);
            writef(output, "store {s} {s}, ptr %{u}\n", type_str, list_at(list, i), generator.reg_count++); 
        }
    } 

    gen_scope(func->body, self_type);

    fputs("\n", output);

}

const char * gen_op(struct Ast * ast, struct Ast * self_type) {
    a_op * op = ast->value;
    
    if (op->op->key == CALL) {
        return gen_call(ast, self_type);
    }
    
    struct Ast * first = op->right;
    struct List * args = init_list(sizeof(struct Ast *));

    if (op->left != NULL) {
        first = op->left;
        list_push(args, op->left);
    }

    list_push(args, op->right);
    
    if (op->definition == NULL) {
        gen_expr(op->right, self_type);
        return NULL;
    }
    a_function * func = op->definition->value;
    
    if (!func->is_inline) {
        logger_log("operator with inlined function definitions are not implemented yet", IR, ERROR);
        exit(1);
    }
    struct Ast * func_first_arg;
    struct Ast * func_param_ast_type = ((a_function *) op->definition->value)->param_type;

    switch (((Type *) func_param_ast_type->value)->intrinsic) {
        case ITuple:
            func_first_arg = list_at(((Tuple_T *) ((Type *) func_param_ast_type->value)->ptr)->types, 0); break;
        default:
            func_first_arg = func_param_ast_type; break;
    }

    gen_inline_function(op->definition, args, get_self_type(ast_get_type_of(first), func_first_arg));

    return NULL;
}

const char * gen_expr_node(struct Ast * ast, struct Ast * self_type) {
    print("reg: {u} | ", generator.reg_count);
    print_ast("node: {s}\n", ast);
    switch (ast->type) {
        case AST_EXPR:
            gen_expr(ast, self_type); break;
        case AST_OP:
        {
            const char * res = gen_op(ast, self_type);
            if (res != NULL) {
                return res;
            }
        } break;
        case AST_LITERAL:
        {
            a_literal * literal = ast->value;
            /* const char * type_str = llvm_ast_type_to_llvm_type(literal->type, self_type); */
            if (literal->literal_type == LITERAL_NUMBER)
                return literal->value;
            else if (literal->literal_type == LITERAL_STRING)
                return literal->value;
        } break;
        case AST_VARIABLE:
        {
            a_variable * var = ast->value;
            writef(output, "%{u} = load {s}, ptr %{u}\n", generator.reg_count++, llvm_ast_type_to_llvm_type(var->type, self_type), var->reg);
        } break;
        default:
            logger_log(format("AST type '{s}' code generation is not implemented for expr node", ast_type_to_str(ast->type)), IR, ERROR);
            exit(1);
    }

    return format("%{u}", generator.reg_count - 1);
}

void gen_expr(struct Ast * ast, struct Ast * self_type) {
    a_expr * expr = ast->value;

    for (int i = 0; i < expr->children->size; ++i) {
        gen_expr_node(list_at(expr->children, i), self_type);
    }
}

void gen_if(struct Ast * ast, struct Ast * self_type) {
    a_if_statement * _if = ast->value;

    unsigned int if_start = generator.reg_count;

    gen_expr(_if->expression, self_type);

    println("reg: {u}", generator.reg_count);
}

void gen_scope(struct Ast * ast, struct Ast * self_type) {
    a_scope * scope = ast->value;
    
    for (int i = 0; i < scope->variables->size; ++i) {
        a_variable * var = ((struct Ast *) list_at(scope->variables, i))->value;
        var->reg = generator.reg_count;

        writef(output, "%{u} = alloca {s}\n", generator.reg_count++, llvm_ast_type_to_llvm_type(var->type, self_type));
    }

    for (int i = 0; i < scope->nodes->size; ++i) {
        struct Ast * node = list_at(scope->nodes, i);
        switch (node->type) {
            case AST_EXPR:
                gen_expr(node, self_type); break;
            case AST_DECLARATION:
                gen_expr(((a_declaration *) node->value)->expression, self_type); break;
            case AST_IF:
                gen_if(node, self_type); break;
            default:
                logger_log(format("AST type '{s}' code generation is not implemented in scope", ast_type_to_str(node->type)), IR, ERROR);
                exit(1);
        }
    }

}

void gen_function_argument_list(struct Ast * ast, struct Ast * self_type) {
    a_expr * expr = ast->value;
 
    for (int i = 0; i < expr->children->size; ++i) {
        if (i != 0) {
            fputs(", ", output);
        }

        a_variable * var = ((struct Ast *) list_at(expr->children, i))->value;
        var->reg = generator.reg_count;

        writef(output, "{s} noundef %{i}", llvm_ast_type_to_llvm_type(
                    ((a_variable *)
                        ((struct Ast *) 
                            list_at(expr->children, i))
                        ->value)
                    ->type, self_type), generator.reg_count++);
    }

    fputs(")", output);
}

void gen_function_with_name(struct Ast * ast, const char * name, struct Ast * self_type) {
    a_function * func = ast->value;

    generator.reg_count = 0, generator.block_count = 0;

    const char * return_type_str = llvm_ast_type_to_llvm_type(func->return_type, self_type);
    
    writef(output, "\ndefine dso_local {s} @{s}(", return_type_str, name); 
    gen_function_argument_list(func->arguments, self_type);

    if (func->is_inline) {
        fputs(" alwaysinline", output);
    }

    fputs(" {\n", output);
    
    a_expr * args = func->arguments->value;
    generator.reg_count += 1;

    for (int i = 0; i < args->children->size; ++i) {
        a_variable * var = ((struct Ast *) list_at(args->children, i))->value;
        var->reg = generator.reg_count;

        const char * type_str = llvm_ast_type_to_llvm_type(var->type, self_type);
        writef(output, "%{u} = alloca {s}\n", generator.reg_count, type_str);
        writef(output, "store {s} %{u}, ptr %{u}\n", type_str, i, generator.reg_count++);
    }

    gen_scope(func->body, self_type);
 
    fputs("br label %exit\n\nexit:\n", output);
    writef(output, "ret {s} %{u}\n", 
            return_type_str, generator.reg_count - 1);

    fputs("}\n", output);
}


void gen_function(struct Ast * ast, struct Ast * self_type) {
    gen_function_with_name(ast, ((a_function *) ast->value)->name, self_type);
}

void gen_structs(struct Ast * ast, struct Ast * self_type) {
    a_struct * _struct = ast->value;

    writef(output, "%struct.{s} = type {c}", _struct->name, '{');

    for (int i = 0; i < _struct->variables->size; ++i) {
        struct Ast * node = list_at(_struct->variables, i);
        if (i != 0)
            fputc(',', output);
        writef(output, " {s}", llvm_ast_type_to_llvm_type(((a_variable *) node->value)->type, self_type));
    }


    writef(output, " }\n");

    for (int i = 0; i < _struct->functions->size; ++i) {
        struct Ast * node = list_at(_struct->functions, i);
        a_function * func = node->value;
        ASSERT1(func->name != NULL);
        gen_function_with_name(node, format("{s}_{s}", _struct->name, func->name), _struct->type);
    }

}

void gen_impl(struct Ast * ast) {
    struct Ast * type_ast, * function_ast;
    a_impl * impl = ast->value;

    a_type * types = impl->type->value;
    Tuple_T * tuple = types->ptr;

    for (int i = 0; i < tuple->types->size; ++i) {
        type_ast = list_at(tuple->types, i);
        a_type * type = type_ast->value;

        for (int j = 0; j < impl->members->size; ++j) {
            function_ast = list_at(impl->members, j);
            a_function * func = function_ast->value;
            if (func->name[0] != '#')
                gen_function_with_name(function_ast, format("{s}_{s}", type_to_str(type), func->name), type_ast);
        }
    }

}

void gen_module(struct Ast * ast) {
    a_module * module = ast->value;
    
    for (int i = 0; i < module->structures->size; ++i) {
        gen_structs(list_at(module->structures, i), NULL);
    }

    for (int i = 0; i < module->variables->size; ++i) {
        print_ast(format("{u!} global: {s}\n"), list_at(module->variables, i));
    }
     
    for (int i = 0; i < module->functions->size; ++i) {
        gen_function(list_at(module->functions, i), NULL);
    }

    for (int i = 0; i < module->impls->size; ++i) {
        gen_impl(list_at(module->impls, i));
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
