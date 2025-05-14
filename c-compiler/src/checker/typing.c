#include "checker/typing.h"
#include "parser/types.h"
#include <stdlib.h>
#include <sys/types.h>

static u_int32_t next_var_id = 0;

Type * create_type_variable() {
	Variable_T * var = malloc(sizeof(Variable_T));
	var->ID = next_var_id++;
	var->type = NULL;

	// Type * type = malloc(size_t size)
}
