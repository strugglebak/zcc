#ifndef __GENERATOR_H__
#define __GENERATOR_H__

int generate_label();
int interpret_ast_with_register(
  struct ASTNode *node,
  int if_label,
  int loop_start_label,
  int loop_end_label,
  int parent_ast_operation
);
void generate_preamble_code();
void generate_postamble_code();
void generate_clearable_registers(int keep_register_index);
int generate_global_string_code(char *string_value, int is_append_string);
void generate_global_string_code_end();
void generate_global_symbol(struct SymbolTable *t);
void generate_reset_local_variables();
int generate_get_local_offset(int primitive_type);
int generate_get_primitive_type_size(int primitive_type);
int generate_align(int primitive_type, int offset, int direction);

#endif
