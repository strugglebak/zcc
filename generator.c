#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "data.h"
#include "ast.h"
#include "generator.h"
#include "generator_core.h"
#include "helper.h"

// 这个 label 是为了在汇编 code 中生成类似 L1, L2 的代码用的
// 目的是为了代码间的跳转
// 比如 jmp L1, jmp L2 之类去执行
int generate_label() {
  static int id = 1;
  return id++;
}

static int interpret_if_ast_with_register(struct ASTNode *node) {
  int label_start, label_end;

  label_start = generate_label();
  if (node->right) label_end = generate_label();

  // 解析 if 中的 condition 条件语句，并生成对应的汇编代码
  // 这个条件语句最终的执行的结果是 0，并跳转到 label_false 存在的地方
  // 这里可以把 label_false 这个整型数值当作一个 register_index 给到对应的寄存器做处理
  interpret_ast_with_register(node->left, label_start, node->operation);
  clear_all_registers();

  // 解析 true 部分的 statements，并生成对应汇编代码
  interpret_ast_with_register(node->middle, NO_REGISTER, node->operation);
  clear_all_registers();

  // 如果有 ELSE 分支，则跳转到 label_end
  if (node->right) register_jump(label_end);

  // 开始写 label_start 定义的代码
  register_label(label_start);
  if (node->right) {
    interpret_ast_with_register(node->right, NO_REGISTER, node->operation);
    clear_all_registers();
    register_label(label_end);
  }

  return NO_REGISTER;
}

/**
 * 形成类似
 * Lstart:
 *         evaluate condition
	         jump to Lend if condition false
	         statements
	         jump to Lstart
   Lend:
   这样的效果
*/
static int interpret_while_ast_with_register(struct ASTNode *node) {
  int label_start, label_end;

  label_start = generate_label();
  label_end = generate_label();

  // 先生成一个 Lstart
  register_label(label_start);

  // 解析 while 中的 condition 条件语句，并生成对应的汇编代码
  // evaluate condition
  // jump to Lend if condition false
  interpret_ast_with_register(node->left, label_end, node->operation);
  clear_all_registers();

  // 解析 while 下面的复合语句
  // statements
  interpret_ast_with_register(node->right, NO_REGISTER, node->operation);
  clear_all_registers();

  // jump to Lstart
  register_jump(label_start);

  // 最后生成一个 Lend
  register_label(label_end);

  return NO_REGISTER;
}

/**
 * 这里主要将 ast 中的代码取出来，然后用汇编的方式进行值的加减乘除
 * 这里加减乘除后返回的是寄存器的标识
 *
*/
int interpret_ast_with_register(
  struct ASTNode *node,
  int register_index,
  int parent_ast_operation
) {
  int left_register, right_register;

  /**
   * 对于两种 ast 做特殊处理
   * 1. if 条件语句
   *           IF
             / |  \_______
            /  |          \
         cond (true stmt1) (false/end stmt2)

     2. glue 复合语句(这个结构是为了把各种语句粘合起来)
               AST_GLUE
              /  \
          AST_GLUE stmt4
            /  \
        AST_GLUE stmt3
          /  \
      stmt1  stmt2

     3. while 语句
            while
           /   |   \
          /    |    \
        cond  NULL  stmt
  */
  switch (node->operation) {
    case AST_IF:
      return interpret_if_ast_with_register(node);
    case AST_GLUE:
      interpret_ast_with_register(node->left, NO_REGISTER, node->operation);
      clear_all_registers();
      interpret_ast_with_register(node->right, NO_REGISTER, node->operation);
      clear_all_registers();
      return NO_REGISTER;
    case AST_WHILE:
      return interpret_while_ast_with_register(node);
    case AST_FUNCTION:
      register_function_preamble(global_symbol_table[node->value.symbol_table_index].name);
      interpret_ast_with_register(node->left, NO_REGISTER, node->operation);
      register_function_postamble(node->value.symbol_table_index);
      return NO_REGISTER;
  }

  if (node->left) {
    left_register = interpret_ast_with_register(node->left, NO_REGISTER, node->operation);
  }
  if (node->right) {
    right_register = interpret_ast_with_register(node->right, left_register, node->operation);
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

    case AST_COMPARE_EQUALS:
    case AST_COMPARE_NOT_EQUALS:
    case AST_COMPARE_LESS_THAN:
    case AST_COMPARE_GREATER_THAN:
    case AST_COMPARE_LESS_EQUALS:
    case AST_COMPARE_GREATER_EQUALS:
      if (parent_ast_operation == AST_IF ||
        parent_ast_operation == AST_WHILE)
        return register_compare_and_jump(
          node->operation, left_register,
          right_register, register_index);
      return register_compare_and_set(node->operation, left_register, right_register);

    case AST_FUNCTION_CALL:
      return register_function_call(left_register, node->value.symbol_table_index);
    case AST_RETURN:
      register_function_return(left_register, current_function_symbol_id);
      return NO_REGISTER;

    case AST_INTEGER_LITERAL:
      return register_load_interger_literal(node->value.interger_value);
    case AST_IDENTIFIER:
      return register_load_value_from_variable(node->value.symbol_table_index);
    case AST_LVALUE_IDENTIFIER:
      return register_store_value_2_variable(register_index, node->value.symbol_table_index);
    case AST_ASSIGNMENT_STATEMENT:
      // 这里所有的生成汇编代码的工作已经结束，返回结果就好
      return right_register;

    case AST_PRINT:
      // 打印左子树的值
      register_print(left_register);
      clear_all_registers();
      return NO_REGISTER;


    case AST_WIDEN:
      return register_widen(left_register, node->left->primitive_type, node->primitive_type);

    default:
      error_with_digital("Unknown AST operator", node->operation);
  }
}

void generate_code(struct ASTNode *node) {
  int register_index;

  // 增加汇编前置代码
  register_preamble();
  register_index = interpret_ast_with_register(node, NO_REGISTER, node->operation);
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

void generate_global_symbol_table_code(int symbol_table_index) {
  register_generate_global_symbol(symbol_table_index);
}
