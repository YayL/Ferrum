#pragma once

#include "parser/AST.h"

ID qualify_symbol(a_symbol * symbol, enum id_type type_to_find);
ID qualify_declaration(Arena declarations, ID declaration_name_id);
