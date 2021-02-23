#ifndef __GENERATOR_CORE_H__
#define __GENERATOR_CORE_H__

void register_preamble();
void register_postamble();

int register_load_interger_literal(int value);
int register_plus(int left_register, int right_register);
int register_minus(int left_register, int right_register);
int register_multiply(int left_register, int right_register);
int register_divide(int left_register, int right_register);

void register_print(int register_index);

#endif
