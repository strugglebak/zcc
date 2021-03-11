#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "data.h"
#include "ast.h"
#include "generator_core.h"

/**
 * 这里主要将 ast 中的代码取出来，然后用汇编的方式进行值的加减乘除
 * 这里加减乘除后返回的是寄存器的标识
*/
int interpret_ast_with_register(struct ASTNode *node) {
  int left_register, right_register;

  if (node->left) {
    left_register = interpret_ast_with_register(node->left);
  }
  if (node->right) {
    right_register = interpret_ast_with_register(node->right);
  }

  switch (node->operation) {
  case AST_PLUS:
    return register_plus(left_register, right_register);
  case AST_MINUS:
    return register_minus(left_register, right_register);
  case AST_MULTIPLY:
    return register_multiply(left_register, right_register);
  case AST_DIVIDE:
    return register_divide(left_register, right_register);
  case AST_INTEGER_LITERAL:
    return register_load_interger_literal(node->value.interger_value);
  default:
    fprintf(stderr, "Unknown AST operator %d\n", node->operation);
    exit(1);
  }
}

void generate_code(struct ASTNode *node) {
  int register_index;

  // 增加汇编前置代码
  register_preamble();
  register_index = interpret_ast_with_register(node);
  register_print(register_index);
  // 增加汇编后置代码
  register_postamble();
}

void generate_preamble_code() {
  register_preamble();
}

void generate_postamble_code() {
  register_postamble();
}

void generate_clearable_registers() {
  clear_all_registers();
}

void generate_printable_code(int register_index) {
  register_print(register_index);
}

void generate_global_symbol_table_code(char *symbol_string) {
  register_generate_global_symbol(symbol_string);
}
