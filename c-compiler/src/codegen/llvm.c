#include "codegen/llvm.h"

#include "parser/AST.h"
#include "tables/registry_manager.h"

const char * llvm_type_to_llvm_type(ID type_id) {
    switch (type_id.type) {
        case ID_VOID_TYPE:
            return "void";
        case ID_NUMERIC_TYPE:
        {
            Numeric_T num = LOOKUP(type_id, Numeric_T);
            if (num.type == NUMERIC_SIGNED || num.type == NUMERIC_UNSIGNED)
                return format("i{u}", num.width);
            else
                return format("f{u}", num.width);
        }
        // case IStruct:
        //     return format("%struct.{s}", interner_lookup_str(type.name_id));
        case ID_REF_TYPE:
            return "ptr";
        case ID_ARRAY_TYPE:
        {
            Array_T array = LOOKUP(type_id, Array_T);
            return format("[{u} x {s}]", array.size, llvm_type_to_llvm_type(array.basetype_id));
        }
        /* case ISelf: */
        /*     return llvm_type_to_llvm_type(self_type->value, NULL); */
        default:
            FATAL("Intrinsic type not implemented: {s}", id_type_to_string(type_id.type));
    }
}

const char * llvm_type_to_llvm_arg_type(ID type_id) {
    switch (type_id.type) {
        // case IStruct:
        //     return format("ptr byval(%struct.{s})", interner_lookup_str(type.name_id));
        default:
            return llvm_type_to_llvm_type(type_id);
    }
}

unsigned int llvm_get_register_of(ID node_id) {
    ASSERT1(ID_IS(node_id, ID_AST_VARIABLE));

    a_variable variable = LOOKUP(node_id, a_variable);
    return variable.reg;
}
