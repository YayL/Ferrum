#include "tables/symbol_table.h"

#include "tables/registry_manager.h"
#include "parser/AST.h"

struct symbol_table symbol_table_init() {
	return (struct symbol_table) {
		.declarations = symbol_map_init(),
		.types  = symbol_map_init(),
		.traits = symbol_map_init(),
		.imports = symbol_map_init(),
	};
}

void _symbol_table_insert(struct symbol_table * table, ID name_id, ID node_id) {
	switch (node_id.type) {
		case ID_AST_DECLARATION:
		case ID_AST_FUNCTION: symbol_map_insert(&table->declarations, name_id, node_id); break;
		case ID_AST_STRUCT: 
		case ID_AST_GROUP:
		case ID_AST_ENUM: symbol_map_insert(&table->types, name_id, node_id); break;
		case ID_AST_TRAIT: symbol_map_insert(&table->traits, name_id, node_id); break;
		case ID_AST_IMPORT: symbol_map_insert(&table->imports, name_id, node_id); break;
		default:
		FATAL("Unable to handle '{s}' insert", id_type_to_string(node_id.type));
	}
}

void symbol_table_insert(struct symbol_table * table, ID node_id) {
	switch (node_id.type) {
		case ID_AST_DECLARATION: {
			a_declaration declaration = LOOKUP(node_id, a_declaration);
			_symbol_table_insert(table, declaration.name_id, node_id);
		} break;
		case ID_AST_FUNCTION: {
			a_function function = LOOKUP(node_id, a_function);
			_symbol_table_insert(table, function.name_id, node_id);
		} break;
		case ID_AST_STRUCT: {
			a_structure structure = LOOKUP(node_id, a_structure);
			_symbol_table_insert(table, structure.name_id, node_id);
		} break;
		case ID_AST_ENUM: {
			a_enumeration enumeration = LOOKUP(node_id, a_enumeration);
			_symbol_table_insert(table, enumeration.name_id, node_id);
		} break;
		case ID_AST_GROUP: {
			a_group group = LOOKUP(node_id, a_group);
			_symbol_table_insert(table, group.name_id, node_id);
		} break;
		case ID_AST_TRAIT: {
			a_trait trait = LOOKUP(node_id, a_trait);
			_symbol_table_insert(table, trait.name_id, node_id);
		} break;
		case ID_AST_SYMBOL: {
			a_symbol symbol = LOOKUP(node_id, a_symbol);
			_symbol_table_insert(table, symbol.name_id, symbol.node_id);
		} break;
		case ID_AST_IMPORT: {
			a_import import = LOOKUP(node_id, a_import);
			_symbol_table_insert(table, import.name_id, node_id);
		} break;
		case ID_AST_IMPL: break;
		default:
			FATAL("Invalid AST type: {s}", id_type_to_string(node_id.type));
	}
}

void _symbol_table_remove(struct symbol_table * table, ID name_id, enum id_type node_type) {
	switch (node_type) {
		case ID_AST_DECLARATION:
		case ID_AST_FUNCTION: symbol_map_remove(&table->declarations, name_id); break;
		case ID_AST_STRUCT: 
		case ID_AST_ENUM: symbol_map_remove(&table->types, name_id); break;
		case ID_AST_TRAIT: symbol_map_remove(&table->traits, name_id); break;
		case ID_AST_IMPORT: symbol_map_remove(&table->imports, name_id); break;
		default:
		FATAL("Unable to handle '{s}' insert", id_type_to_string(node_type));
	}
}

void symbol_table_remove(struct symbol_table * table, ID node_id) {
	switch (node_id.type) {
		case ID_AST_DECLARATION: {
			a_declaration declaration = LOOKUP(node_id, a_declaration);
			_symbol_table_remove(table, declaration.name_id, ID_AST_DECLARATION);
		} break;
		case ID_AST_FUNCTION: {
			a_function function = LOOKUP(node_id, a_function);
			_symbol_table_remove(table, function.name_id, ID_AST_FUNCTION);
		} break;
		case ID_AST_STRUCT: {
			a_structure structure = LOOKUP(node_id, a_structure);
			_symbol_table_remove(table, structure.name_id, ID_AST_STRUCT);
		} break;
		case ID_AST_ENUM: {
			a_enumeration enumeration = LOOKUP(node_id, a_enumeration);
			_symbol_table_remove(table, enumeration.name_id, ID_AST_ENUM);
		} break;
		case ID_AST_TRAIT: {
			a_trait trait = LOOKUP(node_id, a_trait);
			_symbol_table_remove(table, trait.name_id, ID_AST_TRAIT);
		} break;
		case ID_AST_SYMBOL: {
			a_symbol symbol = LOOKUP(node_id, a_symbol);
			_symbol_table_remove(table, symbol.name_id, symbol.node_id.type);
		} break;
		case ID_AST_IMPORT: {
			a_import import = LOOKUP(node_id, a_import);
			_symbol_table_remove(table, import.name_id, ID_AST_IMPORT);
		} break;
		case ID_AST_IMPL: break;
		default:
			FATAL("Invalid AST type: {s}", id_type_to_string(node_id.type));
	}
}

void symbol_table_extend(struct symbol_table * table, const struct symbol_table other) {
	symbol_map_extend(&table->declarations, &other.declarations);
	symbol_map_extend(&table->types, &other.types);
	symbol_map_extend(&table->traits, &other.traits);
	symbol_map_extend(&table->imports, &other.imports);
}

void symbol_table_clear(struct symbol_table * table) {
	symbol_map_clear(&table->declarations);
	symbol_map_clear(&table->types);
	symbol_map_clear(&table->traits);
	symbol_map_clear(&table->imports);
}
