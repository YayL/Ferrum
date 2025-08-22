#pragma once

#include "checker/context.h"

struct module {
	struct AST * module;
	Context module_context;
};

typedef struct module_registry {

} ModuleRegistry;
