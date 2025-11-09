#include "checker/functions.h"

#include "checker/typing.h"
#include "parser/AST.h"
#include "tables/registry_manager.h"

#include "checker/context.h"
#include "tables/member_functions.h"

void resolve_function_from_call(ID node_id) {
    ASSERT1(ID_IS(node_id, ID_AST_OP));
    a_operator * op = lookup(node_id);

    ID left_id = op->left_id, right_id = op->right_id;
    ASSERT1(ID_IS(left_id, ID_AST_SYMBOL));
    a_symbol function_symbol = LOOKUP(left_id, a_symbol);

    ID args_type_id = ast_to_type(right_id);
    Arena candidates = context_lookup_all_declarations(function_symbol.name_id);

    FRResult result = frsolver_solve(frsolver_init(function_symbol.name_id, args_type_id, function_symbol.info.scope_id, candidates));

    if (ID_IS_INVALID(result.function_id)) {
        ERROR("Function {s}{s} is not defined", interner_lookup_str(function_symbol.name_id)._ptr, type_to_str(args_type_id));
        exit(1);
    }

    op->definition.function_id = result.function_id;
    op->type_id = result.function_return_type_id;
}

void resolve_function_from_operator(ID node_id) {
    ASSERT1(ID_IS(node_id, ID_AST_OP));
    a_operator * op = lookup(node_id);

    ID left_id = op->left_id, right_id = op->right_id;

    Arena tuple_types = arena_init(sizeof(ID));

    if (op->op.mode == BINARY) {
        ARENA_APPEND(&tuple_types, ast_get_type_of(left_id));
    }
    ARENA_APPEND(&tuple_types, ast_get_type_of(right_id));

    ID name_id = operator_get_intern_id(op->op.key);

    ID args_type_id = type_from_arena(tuple_types);
    Arena candidates = member_function_index_lookup(name_id);

    FRResult result = frsolver_solve(frsolver_init(name_id, args_type_id, op->info.scope_id, candidates));

    if (ID_IS_INVALID(result.function_id)) {
        ERROR("Operator '{s}'{s} is not defined for {s}", op->op.str, operator_get_runtime_name(op->op.key), type_to_str(args_type_id));
        exit(1);
    }

    op->definition.function_id = result.function_id;
    op->type_id = result.function_return_type_id;

    println("type: {s}, {s}", type_to_str(op->type_id), id_type_to_string(op->type_id.type));
    println("args: {s}", type_to_str(result.args_type_id));
    print_ast_tree(result.function_id);
    print_ast_tree(node_id);

    if (ID_IS(op->type_id, ID_ARRAY_TYPE)) {
        exit(0);
    }
}

ID get_member_function(const char * member_name, ID args_type_id, ID return_type_id, ID scope_id) {
    ASSERT1(!ID_IS_INVALID(args_type_id));
    ASSERT1(!ID_IS_INVALID(return_type_id));


    return INVALID_ID;
}
