#include "codegen/gen.h"
#include "codegen/AST.h"
#include "codegen/llvm.h"
#include "parser/types.h"

FILE * output = NULL;
struct Generator generator = {0};

// unsigned int gen_new_register() {
//     return generator.reg_count++;
// }
//
// void gen_write(const char * src) {
//     fputs(src, output);
// }
//
// void gen_call_with_info(const char * name, const char * ret_type, struct List * arg_types, struct List * arg_values, struct AST * self_type) {
//     writef(output, "%{u} = call {s} @{s}(", generator.reg_count++, ret_type, name);
//
//     for (int i = 0; i < arg_types->size; ++i) {
//         if (i != 0)
//             fputs(", ", output);
//         struct AST * type = list_at(arg_types, i);
//         writef(output, "{s} noundef {s}", llvm_ast_type_to_llvm_arg_type(list_at(arg_types, i), self_type), list_at(arg_values, i));
//     }
//
//     fputs(")\n", output);
// }
//
// const char * gen_call(struct AST * ast, struct AST * self_type) {
//     a_op op = ast->value.operator;
//
//     if (op.left->value.variable.name[0] == '#') {
//         return gen_builtin(ast, self_type);
//     }
//
//     a_expr args = op.right->value.expression;
//     a_function func = op.definition->value.function;
//
//     struct List * parameters = init_list(sizeof(char *));
//
//     const unsigned int size = args.children->size;
//
//     for (int i = 0; i < size; ++i) {
//         list_push(parameters, (void *) gen_expr_node(list_at(args.children, i), self_type));
//     }
//    
//     gen_call_with_info(func.name, llvm_ast_type_to_llvm_type(op.type, self_type), ast_to_ast_type_list(func.param_type), parameters, self_type);
//     return NULL;
// }
//
// void gen_inline_function(struct AST * ast, struct List * arguments, struct AST * self_type) {
//     a_function func = ast->value.function;
//
//     struct List * params = ast_to_ast_type_list(func.param_type);
//
//     struct List * list = init_list(sizeof(char *));
//
//     for (int i = 0; i < arguments->size; ++i) {
//         list_push(list, (void *) gen_expr_node(list_at(arguments, i), self_type));
//     }
//
//     writef(output, "\n; inline call: {s}\n", func.name);
//
//     for (int i = 0; i < arguments->size; ++i) {
//         const char * type_str = llvm_ast_type_to_llvm_type(list_at(params, i), self_type);
//         struct AST * param = list_at(func.arguments->value.expression.children, i),
//                    * arg = list_at(arguments, i);
//    
//         if (arg->type == AST_VARIABLE) {
//             param->value.variable.reg = arg->value.variable.reg;
//         } else {
//             param->value.variable.reg = generator.reg_count;
//
//             writef(output, "%{u} = alloca {s}\n", generator.reg_count, type_str);
//             writef(output, "store {s} {s}, ptr %{u}\n", type_str, list_at(list, i), generator.reg_count++); 
//         }
//     } 
//
//     gen_scope(func.body, self_type);
//
//     fputs("\n", output);
//
// }
//
// const char * gen_op(struct AST * ast, struct AST * self_type) {
//     a_op op = ast->value.operator;
//    
//     if (op.op->key == CALL) {
//         return gen_call(ast, self_type);
//     }
//    
//     struct AST * first = op.right;
//     struct List * args = init_list(sizeof(struct AST *));
//
//     if (op.left != NULL) {
//         first = op.left;
//         list_push(args, op.left);
//     }
//
//     list_push(args, op.right);
//    
//     if (op.definition == NULL) {
//         gen_expr(op.right, self_type);
//         return NULL;
//     }
//     a_function func = op.definition->value.function;
//    
//     if (!func.is_inline) {
//         FATAL("operator with inlined function definitions are not implemented yet");
//     }
//     struct AST * func_first_arg;
//     struct AST * func_param_ast_type = op.definition->value.function.param_type;
//
//     switch (func_param_ast_type->value.type.intrinsic) {
//         case ITuple:
//             func_first_arg = list_at(func_param_ast_type->value.type.value.tuple.types, 0); break;
//         default:
//             func_first_arg = func_param_ast_type; break;
//     }
//
//     /* gen_inline_function(op->definition, args, get_self_type(ast_get_type_of(first), func_first_arg)); */
//
//     return NULL;
// }
//
// const char * gen_expr_node(struct AST * ast, struct AST * self_type) {
//     print("reg: {u} | ", generator.reg_count);
//     print_ast("node: {s}\n", ast);
//     switch (ast->type) {
//         case AST_EXPR:
//             gen_expr(ast, self_type); break;
//         case AST_OP:
//         {
//             const char * res = gen_op(ast, self_type);
//             if (res != NULL) {
//                 return res;
//             }
//         } break;
//         case AST_LITERAL:
//         {
//             a_literal literal = ast->value.literal;
//             /* const char * type_str = llvm_ast_type_to_llvm_type(literal->type, self_type); */
//             if (literal.literal_type == LITERAL_NUMBER)
//                 return literal.value;
//             else if (literal.literal_type == LITERAL_STRING)
//                 return literal.value;
//         } break;
//         case AST_VARIABLE:
//         {
//             a_variable var = ast->value.variable;
//             writef(output, "%{u} = load {s}, ptr %{u}\n", generator.reg_count++, llvm_ast_type_to_llvm_type(var.type, self_type), var.reg);
//         } break;
//         default:
//             FATAL("AST type '{s}' code generation is not implemented for expr node", ast_type_to_str(ast->type));
//     }
//
//     return format("%{u}", generator.reg_count - 1);
// }
//
// void gen_expr(struct AST * ast, struct AST * self_type) {
//     a_expr expr = ast->value.expression;
//
//     for (int i = 0; i < expr.children->size; ++i) {
//         gen_expr_node(list_at(expr.children, i), self_type);
//     }
// }
//
// void gen_if_block(a_if_statement _if, unsigned int end, struct AST * self_type) {
//     gen_expr(_if.expression, self_type);
//    
//     unsigned int block_num = generator.reg_count;
//    
//     const char * next_block = _if.next == NULL ? format("%if_end_{u}", end) : format("%else_{u}", block_num);
//
//     if (_if.next == NULL)
//         writef(output, "br i1 %{u}, label %{u}, label {s}\n", generator.reg_count - 1, block_num, next_block);
//     writef(output, "\n{u}:\n", generator.reg_count++);
//
//     gen_scope(_if.body, self_type);
//    
//     if (_if.next != NULL) {
//         writef(output, "else_{u}:\n", block_num);
//         gen_if_block(*_if.next, end, self_type);
//     }
// }
//
// void gen_if(struct AST * ast, struct AST * self_type) {
//     a_if_statement _if = ast->value.if_statement;
//
//     unsigned int end = generator.reg_count;
//     gen_if_block(_if, end, self_type);
//
//     writef(output, "if_end_{u}:\n", end);
// }
//
// void gen_return(struct AST * ast, struct AST * self_type) {    
//     a_return ret = ast->value.return_statement;
//    
//     gen_expr(ret.expression, self_type);
//    
//     writef(output, "ret {s} %{u}\n", generator.ret_type, generator.reg_count - 1);
// }
//
// void gen_scope(struct AST * ast, struct AST * self_type) {
//     a_scope scope = ast->value.scope;
//    
//     for (int i = 0; i < scope.variables->size; ++i) {
//         println("scope var: {i}", i);
//         a_variable * var = &DEREF_AST(list_at(scope.variables, i)).variable;
//         var->reg = generator.reg_count;
//
//         writef(output, "%{u} = alloca {s}\n", generator.reg_count++, llvm_ast_type_to_llvm_type(var->type, self_type));
//     }
//
//     for (int i = 0; i < scope.nodes->size; ++i) {
//         struct AST * node = list_at(scope.nodes, i);
//         switch (node->type) {
//             case AST_EXPR:
//                 gen_expr(node, self_type); break;
//             case AST_DECLARATION:
//                 gen_expr(node->value.declaration.expression, self_type); break;
//             case AST_IF:
//                 gen_if(node, self_type); break;
//             case AST_RETURN:
//                 gen_return(node, self_type); break;
//             default:
//                 FATAL("AST type '{s}' code generation is not implemented in scope", ast_type_to_str(node->type));
//         }
//     }
//
// }
//
// void gen_function_argument_list(struct AST * ast, struct AST * self_type) {
//     a_expr expr = ast->value.expression;
// 
//     for (int i = 0; i < expr.children->size; ++i) {
//         if (i != 0) {
//             fputs(", ", output);
//         }
//
//         a_variable * var = &DEREF_AST(list_at(expr.children, i)).variable;
//         var->reg = generator.reg_count;
//
//         writef(output, "{s} noundef %{i}", llvm_ast_type_to_llvm_type(DEREF_AST(list_at(expr.children, i)).variable.type, self_type), generator.reg_count++);
//     }
//
//     fputs(")", output);
// }
//
// void gen_function_with_name(struct AST * ast, const char * name, struct AST * self_type) {
//     a_function func = ast->value.function;
//
//     generator.reg_count = 0, generator.block_count = 0;
//
//     const char * return_type_str = llvm_ast_type_to_llvm_type(func.return_type, self_type);
//     generator.ret_type = llvm_ast_type_to_llvm_type(func.return_type, self_type);
//    
//     writef(output, "\ndefine dso_local {s} @{s}(", return_type_str, name);
//     gen_function_argument_list(func.arguments, self_type);
//
//     if (func.is_inline) {
//         fputs(" alwaysinline", output);
//     }
//
//     fputs(" {\n", output);
//    
//     a_expr args = func.arguments->value.expression;
//     generator.reg_count += 1;
//
//     for (int i = 0; i < args.children->size; ++i) {
//         a_variable * var = &DEREF_AST(list_at(args.children, i)).variable;
//         var->reg = generator.reg_count;
//
//         const char * type_str = llvm_ast_type_to_llvm_type(var->type, self_type);
//         writef(output, "%{u} = alloca {s}\n", generator.reg_count, type_str);
//         writef(output, "store {s} %{u}, ptr %{u}\n", type_str, i, generator.reg_count++);
//     }
//
//     gen_scope(func.body, self_type);
//
// 
//     fputs("br label %exit\n\nexit:\n", output);
//     writef(output, "ret {s} %{u}\n", 
//             return_type_str, generator.reg_count - 1);
//
//     fputs("}\n", output);
// }
//
//
// void gen_function(struct AST * ast, struct AST * self_type) {
//     gen_function_with_name(ast, ast->value.function.name, self_type);
// }
//
// void gen_structs(struct AST * ast, struct AST * self_type) {
//     a_struct _struct = ast->value.structure;
//
//     writef(output, "%struct.{s} = type {c}", _struct.name, '{');
//
//     for (int i = 0; i < _struct.variables->size; ++i) {
//         struct AST * node = list_at(_struct.variables, i);
//         if (i != 0)
//             fputc(',', output);
//         writef(output, " {s}", llvm_ast_type_to_llvm_type(node->value.variable.type, self_type));
//     }
//
//
//     writef(output, " }\n");
//
//     for (int i = 0; i < _struct.functions->size; ++i) {
//         struct AST * node = list_at(_struct.functions, i);
//         a_function func = node->value.function;
//         ASSERT1(func.name != NULL);
//         gen_function_with_name(node, format("{s}_{s}", _struct.name, func.name), _struct.type);
//     }
//
// }
//
// void gen_impl(struct AST * ast) {
//     struct AST * type_ast, * function_ast;
//     a_impl impl = ast->value.implementation;
//
//     a_type types = impl.type->value.type;
//     Tuple_T tuple = types.value.tuple;
//
//     for (int i = 0; i < tuple.types->size; ++i) {
//         type_ast = list_at(tuple.types, i);
//         a_type type = type_ast->value.type;
//
//         for (int j = 0; j < impl.members->size; ++j) {
//             function_ast = list_at(impl.members, j);
//             a_function func = function_ast->value.function;
//             if (func.name[0] != '#')
//                 gen_function_with_name(function_ast, format("{s}_{s}", type_to_str(type), func.name), type_ast);
//         }
//     }
//
// }
//
// void gen_module(struct AST * ast) {
//     a_module module = ast->value.module;
//    
//     for (int i = 0; i < module.structures->size; ++i) {
//         gen_structs(list_at(module.structures, i), NULL);
//     }
//
//     for (int i = 0; i < module.variables->size; ++i) {
//         print_ast(format("{u!} global: {s}\n"), list_at(module.variables, i));
//     }
//     
//     for (int i = 0; i < module.functions->size; ++i) {
//         gen_function(list_at(module.functions, i), NULL);
//     }
//
//     for (int i = 0; i < module.impls->size; ++i) {
//         gen_impl(list_at(module.impls, i));
//     }
//
// }
//
// void gen(FILE * fp, struct AST * ast) {
//     ASSERT1(ast->type == AST_ROOT);
//     ASSERT1(fp != NULL);
//     a_root root = ast->value.root;
//     output = fp;
//
//     for (int i = 0; i < root.modules->size; ++i) {
//         gen_module(list_at(root.modules, i));
//     }
// }
