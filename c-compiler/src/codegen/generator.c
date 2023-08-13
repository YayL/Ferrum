#include "codegen/gen.h"

struct Generator * init_generator() {
    struct Generator * gen = malloc(sizeof(struct Generator));
    
    gen->globals = init_string("");
    gen->current = init_string("");

    return gen;
}

struct Ast * get_function(struct Ast * scope, char * name) {

    while (scope->type != AST_MODULE) {
        scope = scope->scope;
    }
    
    a_module * module = scope->value;
    a_function * func;

    for (int i = 0; i < module->functions->size; ++i) {
        func = ((struct Ast * ) list_at(module->functions, i))->value;
        if (!strcmp(func->name, name))
            return list_at(module->functions, i);
    }
    
    return NULL;
}

char * get_operand(struct Generator * gen, struct Ast * ast) {
    switch (ast->type) {
        case AST_OP:
            return format("{i}", gen->reg_count - 1);
        case AST_LITERAL:
            return ((a_literal *) ast->value)->value;
        case AST_VARIABLE:
            return format("%{i}", ((a_variable *) ast->value)->reg);
    }
}

void gen_allocate(String * str, int reg, char * type) {
    string_append(str, format("%{i} = alloca {s}\n", reg, type));
}

void gen_allocate_variable(String * str, struct Generator * gen, struct Ast * ast) {
    a_variable * variable = ast->value;

    variable->reg = gen->reg_count++;
    gen_allocate(str, variable->reg, variable->type ? variable->type : "i32");
}

void gen_load(struct Generator * gen, int src) {
    string_append(gen->current, format("%{i} = load i32, ptr %{i}\n", gen->reg_count++, src));
}

void gen_store_variable(String * str, int dest, int src) {
    string_append(str, format("store i32 %{i}, ptr %{i}\n", src, dest));
}

void gen_store(String * str, int dest, char * value) {
    string_append(str, format("store i32 {s}, ptr %{i}\n", value, dest));
}

void gen_op_inst(String * str, char * inst, int res, int left, int right) {
    char * fmt = format("%{i} = {s} i32 %{i}, %{i}\n", res, inst, left, right);
    string_append(str, fmt);
    free(fmt);
}

void gen_literal(struct Generator * gen, struct Ast * ast) {
    a_literal * literal = ast->value;
    gen_allocate(gen->current, gen->reg_count++, "i32");
    gen_store(gen->current, gen->reg_count - 1, literal->value);
    gen_load(gen, gen->reg_count - 1);
}

int gen_expr_node(struct Generator * gen, struct Ast * ast) {
    switch (ast->type) {
        case AST_OP:
            gen_op(gen, ast);
            break;
        case AST_LITERAL:
            gen_literal(gen, ast);
            break;
        case AST_VARIABLE:
        {
            a_variable * var = ast->value;
            gen_load(gen, var->reg);
            break;
        }
    }

    return gen->reg_count - 1;
}

void gen_call(struct Generator * gen, struct Ast * ast) {
    struct Ast * node;
    a_op * op = ast->value;
    a_variable * variable = op->left->value;
    a_expr * expr = op->right->value;

    a_function * func = get_function(ast->scope, variable->name)->value;
    String * arguments = init_string("");

    for (int i = 0; i < expr->children->size; ++i) {
        node = list_at(expr->children, i);

        if (i != 0)
            string_append(arguments, ", ");

        switch (node->type) {
            case AST_OP:
                gen_op(gen, node);
                // add expression types
                string_append(arguments, format("{s} noundef %{i}", "i32", gen->reg_count - 1));
                break;
            case AST_VARIABLE:
                gen_expr_node(gen, node);
                string_append(arguments, format("{s} noundef %{i}", ((a_variable *) node->value)->type, gen->reg_count - 1));
                break;
            case AST_LITERAL:
            {
                a_literal * literal = node->value;
                if (literal->type != NUMBER) {
                    logger_log("Only literal type implemented is numeric literals", IR, ERROR);
                    exit(1);
                }

                string_append(arguments, format("i32 noundef {s}", literal->value));
                break;
            }
        }
    }

    string_append(gen->current, format("%{i} = call {s} @{s}({s})\n", gen->reg_count++, func->type, func->name, arguments->_ptr));
    free_string(&arguments);
}

void gen_op(struct Generator * gen, struct Ast * ast) {
    a_op * op = ast->value;
    
    switch (op->op->key) {
        case CALL:
            gen_call(gen, ast);
            break; 
        case INCREMENT:
            gen_op_inst(gen->current, "add", gen->reg_count++, 1, gen_expr_node(gen, op->right));
            break;
        case DECREMENT:
            gen_op_inst(gen->current, "sub", gen->reg_count++, gen_expr_node(gen, op->right), 1);
            break;
        case ADDITION:
            gen_op_inst(gen->current, "add", gen->reg_count++, gen_expr_node(gen, op->left), gen_expr_node(gen, op->right));
            break;
        case SUBTRACTION:
            gen_op_inst(gen->current, "sub", gen->reg_count++, gen_expr_node(gen, op->left), gen_expr_node(gen, op->right));
            break;
        case UNARY_MINUS:
            gen_op_inst(gen->current, "sub", gen->reg_count++, 0, gen_expr_node(gen, op->right));
            break;
        case DIVISION:
            gen_op_inst(gen->current, "sdiv", gen->reg_count++, gen_expr_node(gen, op->left), gen_expr_node(gen, op->right));
            break;
        case MULTIPLICATION:
            gen_op_inst(gen->current, "mul", gen->reg_count++, gen_expr_node(gen, op->left), gen_expr_node(gen, op->right));
            break;
        case LOGICAL_NOT:
            gen_op_inst(gen->current, "icmp eq", gen->reg_count++, 0, gen_expr_node(gen, op->right));
            break;
        case BITWISE_NOT:
            gen_op_inst(gen->current, "sub nsw", gen->reg_count++, 0, gen_expr_node(gen, op->right));
            break;
        case ASSIGNMENT:
            gen_store_variable(gen->current, ((a_variable *) op->left->value)->reg, gen_expr_node(gen, op->right));
            break;
        default:
            println("missed op: {s}", op->op->str);
    }
}

void gen_if(struct Generator * gen, struct Ast * ast) {

}

void gen_while(struct Generator * gen, struct Ast * ast) {

}

void gen_do(struct Generator * gen, struct Ast * ast) {

}

void gen_expr(struct Generator * gen, struct Ast * ast) {
    struct Ast * node;
    a_expr * expr = ast->value;

    for (int i = 0; i < expr->children->size; ++i) {
        gen_expr_node(gen, list_at(expr->children, i));
    }
}

void gen_scope(struct Generator * gen, struct Ast * ast) {
    struct Ast * node;
    a_scope * scope = ast->value;

    String * stores = init_string("");

    for (int i = 0; i < scope->variables->size; ++i) {
        gen_allocate_variable(gen->current, gen, list_at(scope->variables, i));
    }

    for (int i = 0; i < scope->nodes->size; ++i) {
        node = list_at(scope->nodes, i);
        switch (node->type) {
            case AST_DECLARATION:
                gen_expr(gen, ((a_declaration *) node->value)->expression);
                break;
            case AST_EXPR:
                gen_expr(gen, node);
                break;
            case AST_IF:
                gen_if(gen, node);
                break;
            case AST_WHILE:
                gen_while(gen, node);
                break;
            case AST_DO:
                gen_do(gen, node);
                break;
        }
    }
}

void gen_function(struct Generator * gen, struct Ast * ast) {
    struct Ast * node;
    a_function * func = ast->value;
    
    string_append(gen->current, format("define dso_local {s:type} @{s:name}(", func->type, func->name));
    
    String * allocas = init_string(""),
           * stores = init_string("");

    gen->reg_count = func->arguments->size;

    for (int i = 0; i < func->arguments->size; ++i) {
        node = list_at(func->arguments, i);
        a_variable * argument = list_at(func->arguments, i);
        argument->reg = i;
        string_append(gen->current, format("{s:comma or not}{s:type} noundef %{i:ID}", i == 0 ? "" : ", ", func->type, i));
        gen_allocate_variable(allocas, gen, node);
        gen_store_variable(stores, func->arguments->size + i, i);
    }
    
    string_append(gen->current, ") {\nentry:\n");

    string_concat(gen->current, allocas);
    string_concat(gen->current, stores);

    free_string(&allocas);
    free_string(&stores);

    gen_scope(gen, func->body);

    a_scope * body = func->body->value;

    if (body->nodes->size != 0 && ((struct Ast *) list_at(body->nodes, -1))->type == AST_EXPR) {
        string_append(gen->current, format("ret {s} %{i}\n", func->type, gen->reg_count - 1));
    }

    string_append(gen->current, "}\n\n");
}

void gen_global_variable(struct Generator * gen, struct Ast * ast) {
    a_variable * variable = ast->value;

    // code generation here
    string_append(gen->globals, format("@{s:name} = dso_local global {s:type} {i:initial value}\n", variable->name, variable->type, 0));
}

void gen_module(struct Generator * gen, struct Ast * ast) {
    a_module * module = ast->value;

    for (int i = 0; i < module->variables->size; ++i) {
        gen_global_variable(gen, list_at(module->variables, i));
    }

    for (int i = 0; i < module->functions->size; ++i) {
        gen_function(gen, list_at(module->functions, i));
    }
}

void gen(struct Ast * ast) {
    struct Generator * gen = init_generator();
    
    a_root * root = ast->value;

    for (int i = 0; i < root->modules->size; ++i) {
        gen_module(gen, list_at(root->modules, i));
    }
    
    FILE * file = open_file("./build/ferrum.ll", "w");

    fputs(gen->globals->_ptr, file);
    fputs(gen->current->_ptr, file);

    fclose(file);
}
