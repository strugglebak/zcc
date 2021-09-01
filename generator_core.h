#ifndef __GENERATOR_CORE_H__
#define __GENERATOR_CORE_H__


void clear_all_registers();

void register_preamble();
void register_postamble();

int register_load_interger_literal(int value);
int register_plus(int left_register, int right_register);
int register_minus(int left_register, int right_register);
int register_multiply(int left_register, int right_register);
int register_divide(int left_register, int right_register);

void register_print(int register_index);

int register_load_value_from_variable(int symbol_table_index);
int register_store_value_2_variable(int register_index, int symbol_table_index);
void register_generate_global_symbol(int symbol_table_index);

int register_compare_and_set(
  int ast_operation,
  int register_left,
  int register_right
);
int register_compare_and_jump(
  int ast_operation,
  int register_left,
  int register_right,
  int label
);

void register_label(int label);
void register_jump(int label);


void register_function_preamble(int symbol_table_index);
void register_function_postamble(int symbol_table_index);

int register_widen(
  int register_index,
  int old_primitive_type,
  int new_primitive_type);

int register_get_primitive_type_size(int primitive_type);
int register_function_call(int register_index, int symbol_table_index);
void register_function_return(int register_index, int symbol_table_index);

int register_load_identifier_address(int symbol_table_index);
int register_dereference_pointer(int register_index, int primitive_type);

#endif
