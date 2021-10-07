#ifndef __GENERATOR_CORE_H__
#define __GENERATOR_CORE_H__


void clear_all_registers();

void register_preamble();
void register_postamble();

int register_load_interger_literal(int value, int primitive_type);
int register_plus(int left_register, int right_register);
int register_minus(int left_register, int right_register);
int register_multiply(int left_register, int right_register);
int register_divide(int left_register, int right_register);

int register_load_value_from_variable(struct SymbolTable *t, int operation);
int register_load_local_value_from_variable(struct SymbolTable *t, int operation);
int register_store_value_2_variable(int register_index, struct SymbolTable *t);
int register_store_local_value_2_variable(int register_index, struct SymbolTable *t);
void register_generate_global_symbol(struct SymbolTable *t);
void register_generate_global_string(int label, char *string_value);

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


void register_function_preamble(struct SymbolTable *t);
void register_function_postamble(struct SymbolTable *t);

int register_widen(
  int register_index,
  int old_primitive_type,
  int new_primitive_type);

int register_get_primitive_type_size(int primitive_type);
int register_function_call(struct SymbolTable *t, int argument_number);
void register_function_return(int register_index, struct SymbolTable *t);

int register_load_identifier_address(struct SymbolTable *t);
int register_dereference_pointer(int register_index, int primitive_type);
int register_store_dereference_pointer(int left_register, int right_register, int primitive_type);
int register_shift_left_by_constant(int register_index, int value);

int register_load_global_string(int label);

int register_and(int left_register, int right_register);
int register_or(int left_register, int right_register);
int register_xor(int left_register, int right_register);
int register_negate(int register_index);
int register_invert(int register_index);
int register_shift_left(int left_register, int right_register);
int register_shift_right(int left_register, int right_register);
int register_logic_not(int register_index);
int register_to_be_boolean(int register_index, int operaion, int label);

void register_reset_local_variables();
int register_new_local_offset(int primitive_type);

void register_text_section_flag();
void register_data_section_flag();

void register_copy_argument(int register_index, int argument_position);
int register_align(int primitive_type, int offset, int direction);


#endif
