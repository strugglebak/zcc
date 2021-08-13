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

int register_load_value_from_variable(char *identifier);
int register_store_value_2_variable(int register_index, char *identifier);
void register_generate_global_symbol(char *symbol_string);

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


void register_function_preamble(char *name);
void register_function_postamble();

#endif
