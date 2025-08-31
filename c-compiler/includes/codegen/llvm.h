#pragma once

#include "parser/types.h"
#include "common/ID.h"

const char * llvm_type_to_llvm_type(ID type_id);

const char * llvm_type_to_llvm_arg_type(ID type_id);

unsigned int llvm_get_register_of(ID node_id);
