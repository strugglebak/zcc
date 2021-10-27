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

static int interpret_if_ast_with_register(
  struct ASTNode *node,
  int loop_start_label,
  int loop_end_label
) {
  int label_start, label_end;

  label_start = generate_label();
  if (node->right) label_end = generate_label();

  // 解析 if 中的 condition 条件语句，并生成对应的汇编代码
  // 这个条件语句最终的执行的结果是 0，并跳转到 label_false 存在的地方
  // 这里可以把 label_false 这个整型数值当作一个 register_index 给到对应的寄存器做处理
  interpret_ast_with_register(node->left, label_start, NO_LABEL, NO_LABEL, node->operation);
  generate_clearable_registers(NO_REGISTER);

  // 解析 true 部分的 statements，并生成对应汇编代码
  interpret_ast_with_register(node->middle, NO_LABEL, loop_start_label, loop_end_label, node->operation);
  generate_clearable_registers(NO_REGISTER);

  // 如果有 ELSE 分支，则跳转到 label_end
  if (node->right) register_jump(label_end);

  // 开始写 label_start 定义的代码
  register_label(label_start);
  if (node->right) {
    interpret_ast_with_register(node->right, NO_LABEL, NO_LABEL, NO_LABEL, node->operation);
    generate_clearable_registers(NO_REGISTER);
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
  interpret_ast_with_register(node->left, label_end, label_start, label_end, node->operation);
  generate_clearable_registers(NO_REGISTER);

  // 解析 while 下面的复合语句
  // statements
  interpret_ast_with_register(node->right, NO_LABEL, label_start, label_end, node->operation);
  generate_clearable_registers(NO_REGISTER);

  // jump to Lstart
  register_jump(label_start);

  // 最后生成一个 Lend
  register_label(label_end);

  return NO_REGISTER;
}

static int interpret_function_call_with_register(struct ASTNode *node) {
  struct ASTNode *glue_node = node->left;
  int register_index;
  int function_argument_number = 0;

  spill_all_register();

  // 处理如下的 tree
  //                AST_FUNCCALL
  //                 /
  //             AST_GLUE
  //              /   \
  //          AST_GLUE  expr4(4)
  //           /   \
  //       AST_GLUE  expr3(3)
  //        /   \
  //    AST_GLUE  expr2(2)
  //    /    \
  //  NULL  expr1(1)
  while(glue_node) {
    // 先生成相关表达式的汇编代码
    register_index = interpret_ast_with_register(glue_node->right, NO_LABEL, NO_LABEL, NO_LABEL, glue_node->operation);
    // 将其复制到第 n 个函数参数中
    register_copy_argument(register_index, glue_node->ast_node_scale_size);
    // 保留第一个参数
    if (!function_argument_number) function_argument_number = glue_node->ast_node_scale_size;
    glue_node = glue_node->left;
  }

  return register_function_call(node->symbol_table, function_argument_number);
}

static int interpret_switch_ast_with_register(struct ASTNode *node) {
  int *case_value, *case_label, case_count = 0;
  int label_jump_start, label_end, label_default = 0;
  int i, register_index;
  struct ASTNode *c;

  // 为 case value 和与之对应的 case label 创建数组
  // 注意这里的 node 的 integer_value 存的是构建 case tree 时的 case_count
  // 也就是 case 的个数
  case_value = (int *) malloc((node->ast_node_integer_value + 1) * sizeof(int));
  case_label = (int *) malloc((node->ast_node_integer_value + 1) * sizeof(int));

  label_jump_start = generate_label();
  label_end = generate_label();
  label_default = label_end;

  // 先生成 switch 条件语句的汇编代码
  register_index = interpret_ast_with_register(node->left, NO_LABEL, NO_LABEL, NO_LABEL, 0);
  register_jump(label_jump_start);
  generate_clearable_registers(register_index);

  // 遍历 tree 的右节点，为每个 case 生成汇编代码
  for(i = 0, c = node->right; c; i++, c = c->right) {
    // 为每个 case 创建 label
    case_label[i] = generate_label();
    case_value[i] = c->ast_node_integer_value;
    register_label(case_label[i]);
    if (c->operation == AST_DEFAULT)
      label_default = case_label[i];
    else
      case_count++;

    // 为每个 case 生成汇编代码
    if (c->left)
      interpret_ast_with_register(c->left, NO_LABEL, NO_LABEL, label_end, 0);
    generate_clearable_registers(NO_REGISTER);
  }

  // 确保最后一个 case 跳过 switch 表
  register_jump(label_end);

  // 生成 switch 表和 end label
  register_switch(register_index, case_count, label_jump_start, case_label, case_value, label_default);
  register_label(label_end);
  return NO_REGISTER;
}

// 生成三元运算符语句的汇编代码
static int interpret_ternary_ast_with_register(struct ASTNode *node) {
  int label_start, label_end, register_index, expression_register_index;

  label_start = generate_label();
  label_end = generate_label();

  // 先产生条件语句的汇编，这个条件语句后面跟着是跳到 false 情况的
  interpret_ast_with_register(
    node->left,
    label_start,
    NO_LABEL,
    NO_LABEL,
    node->operation);
  generate_clearable_registers(NO_REGISTER);

  // 弄一个寄存器来保存俩表达式的结果
  register_index = allocate_register();

  // 生成 true 表达式的汇编以及 false label
  // 把结果放入上述的寄存器中
  expression_register_index = interpret_ast_with_register(
    node->middle,
    NO_LABEL,
    NO_LABEL,
    NO_LABEL,
    node->operation);
  register_move(expression_register_index, register_index);
  // 这个时候不要 clear 用来保存俩表达式结果的寄存器
  generate_clearable_registers(register_index);

  register_jump(label_end);
  register_label(label_start);

  // 生成 false 表达式的汇编以及 end label
  // 把结果放入上述的寄存器中
  expression_register_index = interpret_ast_with_register(
    node->right,
    NO_LABEL,
    NO_LABEL,
    NO_LABEL,
    node->operation);
  register_move(expression_register_index, register_index);
  // 这个时候不要 clear 用来保存俩表达式结果的寄存器
  generate_clearable_registers(register_index);

  register_label(label_end);

  return register_index;
}

static int interpret_logic_and_or_ast_with_register(struct ASTNode *node) {
  int label_start = generate_label();
  int label_end = generate_label();
  int register_index ;

  register_index = interpret_ast_with_register(
    node->left,
    NO_LABEL,
    NO_LABEL,
    NO_LABEL,
    0);
  register_to_be_boolean(register_index, node->operation, label_start);
  generate_clearable_registers(NO_REGISTER);

  register_index = interpret_ast_with_register(
    node->right,
    NO_LABEL,
    NO_LABEL,
    NO_LABEL,
    0);
  register_to_be_boolean(register_index, node->operation, label_start);
  generate_clearable_registers(register_index);

  if (node->operation == AST_LOGIC_AND) {
    register_load_boolean(register_index, 1);
    register_jump(label_end);
    register_label(label_start);
    register_load_boolean(register_index, 0);
  } else {
    register_load_boolean(register_index, 0);
    register_jump(label_end);
    register_label(label_start);
    register_load_boolean(register_index, 1);
  }

  register_label(label_end);
  return register_index;
}

/**
 * 这里主要将 ast 中的代码取出来，然后用汇编的方式进行值的加减乘除
 * 这里加减乘除后返回的是寄存器的标识
 *
*/
int interpret_ast_with_register(
  struct ASTNode *node,
  int if_label,
  int loop_start_label,
  int loop_end_label,
  int parent_ast_operation
) {
  int left_register = NO_REGISTER, right_register = NO_REGISTER;

  if (!node)
    return NO_REGISTER;

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
      return interpret_if_ast_with_register(node, loop_start_label, loop_end_label);
    case AST_GLUE:
      if (node->left)
        interpret_ast_with_register(node->left, if_label, loop_start_label, loop_end_label, node->operation);
      generate_clearable_registers(NO_REGISTER);
      if (node->right)
        interpret_ast_with_register(node->right, if_label, loop_start_label, loop_end_label, node->operation);
      generate_clearable_registers(NO_REGISTER);
      return NO_REGISTER;
    case AST_WHILE:
      return interpret_while_ast_with_register(node);
    case AST_FUNCTION_CALL:
      return interpret_function_call_with_register(node);
    case AST_FUNCTION:
      register_function_preamble(node->symbol_table);
      interpret_ast_with_register(node->left, NO_LABEL, NO_LABEL, NO_LABEL, node->operation);
      register_function_postamble(node->symbol_table);
      return NO_REGISTER;
    case AST_SWITCH:
      return interpret_switch_ast_with_register(node);
    case AST_TERNARY:
      return interpret_ternary_ast_with_register(node);
    case AST_LOGIC_AND:
    case AST_LOGIC_OR:
      return interpret_logic_and_or_ast_with_register(node);
  }

  if (node->left)
    left_register = interpret_ast_with_register(node->left, NO_LABEL, NO_LABEL, NO_LABEL, node->operation);
  if (node->right)
    right_register = interpret_ast_with_register(node->right, NO_LABEL, NO_LABEL, NO_LABEL, node->operation);

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
          parent_ast_operation == AST_WHILE ||
          parent_ast_operation == AST_TERNARY)
        return register_compare_and_jump( node->operation, left_register, right_register, if_label);
      return register_compare_and_set(node->operation, left_register, right_register);
    case AST_RETURN:
      register_function_return(left_register, current_function_symbol_id);
      return NO_REGISTER;

    case AST_INTEGER_LITERAL:
      return register_load_interger_literal(node->ast_node_integer_value, node->primitive_type);
    case AST_IDENTIFIER:
      //               类似于 * y 这种情况
      //                     | |
      //                     /  \
      // parent_ast_operation  node
      if (node->rvalue ||
          parent_ast_operation == AST_DEREFERENCE_POINTER)
        return register_load_variable(node->symbol_table, node->operation);
      return NO_REGISTER;
    // a += b + c
    // 会被解析成如下
    //       AST_PLUS
    //      /        \
    //  AST_IDENT     AST_ADD
    //  rval a     /     \
    //         AST_IDENT  AST_IDENT
    //          rval b  rval c
    case AST_ASSIGN_PLUS:
    case AST_ASSIGN_MINUS:
    case AST_ASSIGN_MULTIPLY:
    case AST_ASSIGN_DIVIDE:
    case AST_ASSIGN:
      // 处理前面 += -= *= /=
      switch (node->operation) {
        case AST_ASSIGN_PLUS:
          left_register = register_plus(left_register, right_register);
          node->right = node->left;
          break;
        case AST_ASSIGN_MINUS:
          left_register = register_minus(left_register, right_register);
          node->right = node->left;
          break;
        case AST_ASSIGN_MULTIPLY:
          left_register = register_multiply(left_register, right_register);
          node->right = node->left;
          break;
        case AST_ASSIGN_DIVIDE:
          left_register = register_divide(left_register, right_register);
          node->right = node->left;
          break;
      }

      // 处理 =
      // 是赋值给一个变量还是给一个指针赋值?
      // x = y
      // *x = y
      // 在 parser 中 left 和 right 做了交换，所以这里要对 right 的 operation 做判断
      switch (node->right->operation) {
        case AST_IDENTIFIER:
          if (node->right->symbol_table->storage_class == STORAGE_CLASS_GLOBAL ||
              node->right->symbol_table->storage_class == STORAGE_CLASS_STATIC)
            return register_store_value_2_variable(left_register, node->right->symbol_table);
          return register_store_local_value_2_variable(left_register, node->right->symbol_table);
        case AST_DEREFERENCE_POINTER:
          return register_store_dereference_pointer(left_register, right_register, node->right->primitive_type);
        default:
          error_with_digital("Can't AST_ASSIGN in interpret_ast_with_register, operation", node->operation);
      }

    // &
    case AST_IDENTIFIER_ADDRESS:
      return register_load_identifier_address(node->symbol_table);
    // *
    case AST_DEREFERENCE_POINTER:
      if (node->rvalue)
        return register_dereference_pointer(left_register, node->left->primitive_type);
      // 返回上一个回调交给 AST_ASSIGN 处理
      return left_register;

    // 对 char/int/long 类型转换的处理
    case AST_WIDEN:
      return register_widen(left_register, node->left->primitive_type, node->primitive_type);
    // 对 char*/int*/long* 指针类型转换的处理
    case AST_SCALE:
      switch (node->ast_node_scale_size) {
        case 2: return register_shift_left_by_constant(left_register, 1); // 左移 1 位
        case 4: return register_shift_left_by_constant(left_register, 2); // 左移 2 位
        case 8: return register_shift_left_by_constant(left_register, 3); // 左移 3 位
        default:
          right_register = register_load_interger_literal(node->ast_node_scale_size, PRIMITIVE_INT);
          return register_multiply(left_register, right_register);
      }

    // 处理 string
    case AST_STRING_LITERAL:
      return register_load_global_string(node->ast_node_integer_value);

    case AST_AMPERSAND:
      return register_and(left_register, right_register);
    case AST_OR:
      return register_or(left_register, right_register);
    case AST_XOR:
      return register_xor(left_register, right_register);
    case AST_LEFT_SHIFT:
      return register_shift_left(left_register, right_register);
    case AST_RIGHT_SHIFT:
      return register_shift_right(left_register, right_register);
    case AST_POST_INCREASE:
    case AST_POST_DECREASE:
      return register_load_variable(node->symbol_table, node->operation);
    case AST_PRE_INCREASE:
    case AST_PRE_DECREASE:
      return register_load_variable(node->left->symbol_table, node->operation);
    case AST_NEGATE:
      return register_negate(left_register);
    case AST_INVERT:
      return register_invert(left_register);
    case AST_LOGIC_NOT:
      return register_logic_not(left_register);
    case AST_TO_BE_BOOLEAN:
      return register_to_be_boolean(left_register, parent_ast_operation, if_label);
    case AST_BREAK:
      register_jump(loop_end_label);
      return NO_REGISTER;
    case AST_CONTINUE:
      register_jump(loop_start_label);
      return NO_REGISTER;
    case AST_TYPE_CASTING:
      return left_register;

    default:
      error_with_digital("Unknown AST operator", node->operation);
  }

  return NO_REGISTER;
}

void generate_global_symbol(struct SymbolTable *t) {
  register_generate_global_symbol(t);
}

void generate_preamble_code() {
  register_preamble();
}

void generate_postamble_code() {
  register_postamble();
}

void generate_clearable_registers(int keep_register_index) {
  clear_all_registers(keep_register_index);
}

int generate_global_string_code(char *string_value, int is_append_string) {
  int label = generate_label();
  register_generate_global_string(label, string_value, is_append_string);
  return label;
}

void generate_global_string_code_end() {
  register_generate_global_string_end();
}

void generate_reset_local_variables() {
  register_reset_local_variables();
}

int generate_get_primitive_type_size(int primitive_type) {
  return register_get_primitive_type_size(primitive_type);
}

int generate_align(int primitive_type, int offset, int direction) {
  return register_align(primitive_type, offset, direction);
}
