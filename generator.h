#ifndef __GENERATOR_H__
#define __GENERATOR_H__

int generate_label();
int interpret_ast_with_register(
  struct ASTNode *node,
  int register_index,
  int parent_ast_operation);
void generate_preamble_code();
void generate_postamble_code();
void generate_clearable_registers();
void generate_global_symbol_table_code(int symbol_table_index);
int generate_global_string_code(char *string_value);
void generate_global_symbol(int symbol_table_index);
void generate_reset_local_variables();
int generate_get_local_offset(int primitive_type);
int generate_get_primitive_type_size(int primitive_type);

#endif
