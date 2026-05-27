#include "checker/checker.h"

#include "common/ID.h"
#include "parser/AST.h"
#include "checker/context.h"
#include "tables/registry_manager.h"

#include "checker/typing/solver.h"
#include "checker/typing/checking.h"

#include "parser/keywords.h"

void checker_check_struct(Solver * solver, ID node_id) {
    a_structure structure = LOOKUP(node_id, a_structure);

    Marker * marker = solver_allocate(solver, ID_MARKER);
    solver_collect_templates(solver, node_id, 0);

    for (size_t i = 0; i < structure.members.size; ++i) {
        ID member_id = ARENA_GET(structure.members, i, ID);

        // println("{i}) {s}", i + 1, ast_to_string(member_id));
        switch (member_id.type) {
            case ID_AST_SYMBOL: break;
            case ID_AST_FUNCTION: checker_check_function(solver, member_id); break;
            default: FATAL("Invalid member type: {s}", id_type_to_string(member_id.type));
        }
    }

    solver_reset_to_marker(solver, marker->info.id);
}

void checker_check_impl(Solver * solver, ID node_id) {
    WARN("Unimplemented impl");
}

void checker_check_trait(Solver * solver, ID node_id) { 
    WARN("Unimplemented trait");
}

void checker_check_declaration(Solver * solver, ID node_id) {
    WARN("Unimplemented declaration");
}

void checker_check_function(Solver * solver, ID node_id) {
    a_function function = LOOKUP(node_id, a_function);
    context_add_declaration_list(function.arguments);

    Marker * marker = solver_allocate(solver, ID_MARKER);
    solver_collect_templates(solver, node_id, 0);

    solver_collect_where_attributes(solver, function.where);

    if (function.arguments.size > 0) {
        ASSERT1(ID_IS(function.body_id, ID_AST_SCOPE));
        a_scope * function_scope = lookup(function.body_id);
        arena_grow(&function_scope->declarations, function_scope->declarations.size + function.arguments.size);

        for (int i = 0; i < function.arguments.size; ++i) {
            ID child_node_id = ARENA_GET(function.arguments, i, ID);
            ASSERT(ID_IS(child_node_id, ID_AST_SYMBOL), "Function arguments currently only support declarations");
            a_symbol symbol = LOOKUP(child_node_id, a_symbol);
            ASSERT1(symbol.name_ids.size == 1);
            ASSERT1(ID_IS(symbol.node_id, ID_AST_VARIABLE));
            a_variable variable = LOOKUP(symbol.node_id, a_variable);

            TermVar * var = solver_allocate(solver, ID_TERM_VAR);
            var->symbol_id = symbol.info.node_id;
            var->type = solver_replace_templates(variable.type_id);

            ARENA_APPEND(&function_scope->declarations, child_node_id);
        }
    }

    ASSERT1(ID_IS(function.type, ID_FN_TYPE));
    Fn_T fn_type = LOOKUP(function.type, Fn_T);
    if (!check(solver, function.body_id, solver_replace_templates(fn_type.ret_type)) && (!ID_IS(fn_type.ret_type, ID_VOID_TYPE))) {
        ERROR("Function body doesn't match return type");
    }

    solver_reset_to_marker(solver, marker->info.id);
    context_remove_declaration_list(function.arguments);
}

void checker_check_import(Solver * solver, ID node_id) { }

void checker_check_definitions(Solver * solver, ID node_id) {
    ASSERT1(!ID_IS_INVALID(node_id));

    switch (node_id.type) {
        case ID_AST_FUNCTION:
            checker_check_function(solver, node_id); break;
        case ID_AST_DECLARATION:
            checker_check_declaration(solver, node_id); break;
        case ID_AST_STRUCT:
            checker_check_struct(solver, node_id); break;
        case ID_AST_TRAIT:
            checker_check_trait(solver, node_id); break;
        case ID_AST_IMPL:
            checker_check_impl(solver, node_id); break;
        case ID_AST_IMPORT:
            checker_check_import(solver, node_id); break;
        default:
            FATAL("Invalid AST type: {s}", id_type_to_string(node_id.type));
    }
}

void checker_check_module(Solver * solver, ID node_id) {
    ASSERT1(ID_IS(node_id, ID_AST_MODULE));
    a_module module = LOOKUP(node_id, a_module);
    context_enter_module(module);

    INFO("Checking module '{s}'", module.file_path);

    for (int i = 0; i < module.members.size; ++i) {
        checker_check_definitions(solver, ARENA_GET(module.members, i, ID));
    }

    context_exit_module(module);
}

void checker_check(a_root root) {
    ID module_id;

    Solver solver = solver_initialize();

    kh_foreach_value(&root.modules, module_id, {
        checker_check_module(&solver, module_id);
    });

    // perform main function lookup on root.entry_point module
}
