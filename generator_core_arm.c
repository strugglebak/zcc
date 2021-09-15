#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "data.h"
#include "definations.h"
#include "helper.h"
#include "generator_core.h"

// 把大 int 类型数值放入内存中，用一个数组保存起来
// postamble 会用到
#define MAX_INTEGER_NUMBER 1024

int integer_list[MAX_INTEGER_NUMBER] = { 0 };
static int integer_list_index = 0;

static int free_registers[4] = { 1 };
static char *register_list[4] = { "r4", "r5", "r6", "r7" };

static char *compare_list[] =
  { "moveq", "movne", "movlt", "movgt", "movle", "movge" };

static char *inverted_compare_list[] =
  { "movne", "moveq", "movge", "movle", "movgt", "movlt" };

static char *inverted_branch_list[] =
  { "bne", "beq", "bge", "ble", "bgt", "blt" };

// none/void/char/int/long
static int primitive_size[] = { 0, 0, 1, 4, 4, 4, 4, 4 };

void clear_all_registers() {
  free_registers[0] = free_registers[1] = free_registers[2] = free_registers[3] = 1;
}

/**
 * 分配一个寄存器，返回这个寄存器对应的下标
 * 如果已经没有寄存器分配，则抛出异常
*/
static int allocate_register() {
  for (int i = 0; i < GET_ARRAY_LENGTH(free_registers); i++) {
    if (free_registers[i]) {
      free_registers[i] = 0;
      return i;
    }
  }

  error("Out of registers");
  return NO_REGISTER;
}

/**
 * 清空某个寄存器
*/
static void clear_register(int index) {
  if (free_registers[index])
    error_with_digital("Error trying to clear registers", index);
  free_registers[index] = 1;
}

/**
 * 设置大 int 型数值在 label 下的 offset
*/
static void set_large_integer_offset(int value) {
  int offset = -1;
  for (int i = 0; i < integer_list_index; i++) {
    if (value == integer_list[i]) {
      offset = i * 4;
      break;
    }
  }
  // 如果不在列表中，则直接加入
  if (offset < 0) {
    offset = integer_list_index * 4;
    if (integer_list_index == MAX_INTEGER_NUMBER)
      error("Out of max integer number in set_large_integer_offset()");
    integer_list[integer_list_index++] = value;
  }
  // 用 offset 写入 r3 这个寄存器
  fprintf(output_file, "\tldr\tr3, .L3+%d\n", offset);
}

/**
 * 设置变量在 label 下的 offset
*/
static void set_variable_offset(int symbol_table_index) {
  int offset = 0;
  for (int i = 0; i < symbol_table_index; i++) {
    if (global_symbol_table[i].structural_type == STRUCTURAL_VARIABLE)
      offset += 4;
  }
  // 用 offset 写入 r3 这个寄存器
  fprintf(output_file, "\tldr\tr3, .L2+%d\n", offset);
}

/**
 * 汇编前置代码，写入到 output_file 中
*/
void register_preamble() {
  clear_all_registers();
  fputs("\t.text\n", output_file);
}

/**
 * 汇编后置代码，写入到 output_file 中
*/
void register_postamble() {
  fprintf(output_file, ".L2:\n");
  for (int i = 0; i < global_symbol_table_index; i++) {
    if (global_symbol_table[i].structural_type == STRUCTURAL_VARIABLE)
      fprintf(output_file, "\t.word %s\n", global_symbol_table[i].name);
  }
  fprintf(output_file, ".L3:\n");
  for (int i = 0; i < integer_list_index; i++) {
    fprintf(output_file, "\t.word %d\n", integer_list[i]);
  }
}

/**
 * 把 value 放入某个寄存器中，并返回该寄存器的下标
*/
int register_load_interger_literal(int value, int primitive_type) {
  int register_index = allocate_register();
  if (value <= 1000)
    fprintf(output_file, "\tmov\t%s, #%d\n", register_list[register_index], value);
  else {
    set_large_integer_offset(value);
    fprintf(output_file, "\tldr\t%s, [r3]\n", register_list[register_index]);
  }
  return register_index;
}

/**
 * 两个寄存器相加，并将其放入其中一个寄存器中
*/
int register_plus(int left_register, int right_register) {
  fprintf(output_file,
    "\tadd\t%s, %s, %s\n",
    register_list[right_register],
    register_list[left_register],
    register_list[right_register]);
  clear_register(left_register);
  return right_register;
}

/**
 * 两个寄存器相减，并将其放入其中一个寄存器中
*/
int register_minus(int left_register, int right_register) {
  fprintf(output_file,
    "\tsub\t%s, %s, %s\n",
    register_list[left_register],
    register_list[left_register],
    register_list[right_register]);
  clear_register(right_register);
  return left_register;
}

/**
 * 两个寄存器相乘，并将其放入其中一个寄存器中
*/
int register_multiply(int left_register, int right_register) {
  fprintf(output_file,
    "\tmul\t%s, %s, %s\n",
    register_list[right_register],
    register_list[left_register],
    register_list[right_register]);
  clear_register(left_register);
  return right_register;
}

/**
 * 两个寄存器相除，这里处理比较麻烦
 * 先把值放入 rax 寄存器
 * 进行除法运算后，再从 rax 寄存器中把结果拿出来，放入 left_register
*/
int register_divide(int left_register, int right_register) {
  fprintf(output_file, "\tmov\tr0, %s\n", register_list[left_register]);
  fprintf(output_file, "\tmov\tr1, %s\n", register_list[right_register]);
  fprintf(output_file, "\tbl\t__aeabi_idiv\n");
  fprintf(output_file, "\tmov\t%s, r0\n", register_list[left_register]);
  clear_register(right_register);
  return left_register;
}

/**
 * 打印指定的寄存器
*/
void register_print(int register_index) {
  fprintf(output_file, "\tmov\tr0, %s\n", register_list[register_index]);
  fprintf(output_file, "\tbl\tregister_print\n");
  fprintf(output_file, "\tnop\n");
  clear_register(register_index);
}

/**
 * 将一个变量的值保存到寄存器中
*/
int register_load_value_from_variable(int symbol_table_index) {
  int register_index = allocate_register();
  set_variable_offset(symbol_table_index);
  fprintf(output_file, "\tldr\t%s, [r3]\n", register_list[register_index]);
  return register_index;
}

/**
 * 将寄存器中的值保存到一个变量中
*/
int register_store_value_2_variable(int register_index, int symbol_table_index) {
  struct SymbolTable t = global_symbol_table[symbol_table_index];
  set_variable_offset(symbol_table_index);
  switch (t.primitive_type) {
    case PRIMITIVE_CHAR:
      fprintf(output_file, "\tstrb\t%s, [r3]\n",
       register_list[register_index]);
      break;
    case PRIMITIVE_INT:
    case PRIMITIVE_LONG:
    case PRIMITIVE_CHAR_POINTER:
    case PRIMITIVE_INT_POINTER:
    case PRIMITIVE_LONG_POINTER:
      fprintf(output_file, "\tstr\t%s, [r3]\n",
      register_list[register_index]);
      break;
    default:
      error_with_digital("Bad type in register_store_value_2_variable:", t.primitive_type);
  }
  return register_index;
}

/**
 * 创建全局变量
*/
void register_generate_global_symbol(int symbol_table_index) {
  struct SymbolTable t = global_symbol_table[symbol_table_index];
  int primitive_type_size
    = register_get_primitive_type_size(t.primitive_type);
  fprintf(output_file, "\t.data\n" "\t.globl\t%s\n", t.name);
  fprintf(output_file, "%s:", t.name);
  for (int i = 0; i < t.size; i++) {
    switch(primitive_type_size) {
      case 1: fprintf(output_file, "\t.byte\t0\n"); break;
      case 4: fprintf(output_file, "\t.long\t0\n"); break;
      default: fatald("Unknown typesize in register_generate_global_symbol: ", primitive_type_size);
    }
  }
}

/**
 * 创建全局字符串
*/
void register_generate_global_string(int label, char *string_value) {
  register_label(label);
  for (char *p = string_value; *p; p++) {
    fprintf(output_file, "\t.byte\t%d\n", *p);
  }
  fprintf(output_file, "\t.byte\t0\n");
}

/**
 * 比较两寄存器，如果是 true 则进行 set
*/
int register_compare_and_set(
  int ast_operation,
  int register_left,
  int register_right
) {
  if (ast_operation < AST_COMPARE_EQUALS
    || ast_operation > AST_COMPARE_GREATER_EQUALS)
    error("Bad ast operaion in register_compare_and_set function");

  fprintf(output_file, "\tcmp\t%s, %s\n",
    register_list[register_right],
    register_list[register_left]);

  fprintf(output_file, "\t%s\t%s, #1\n",
    compare_list[ast_operation - AST_COMPARE_EQUALS],
    register_list[register_right]);

  fprintf(output_file, "\t%s\t%s, #0\n",
    inverted_compare_list[ast_operation - AST_COMPARE_EQUALS],
    register_list[register_right]);

  fprintf(output_file, "\tuxtb\t%s, %s\n",
    register_list[register_right],
    register_list[register_right]);

  clear_register(register_left);
  return register_right;
}

/**
 * 比较俩寄存器，如果是 false 则进行 jmp
*/
int register_compare_and_jump(
  int ast_operation,
  int register_left,
  int register_right,
  int label
) {
  if (ast_operation < AST_COMPARE_EQUALS
    || ast_operation > AST_COMPARE_GREATER_EQUALS)
    error("Bad ast operaion in register_compare_and_jump function");

  fprintf(output_file, "\tcmp\t%s, %s\n",
    register_list[register_left],
    register_list[register_right]);
  fprintf(output_file, "\t%s\tL%d\n",
    inverted_branch_list[ast_operation - AST_COMPARE_EQUALS],
    label);

  clear_all_registers();
  return NO_REGISTER;
}

/**
 * 创建 label
*/
void register_label(int label) {
  fprintf(output_file, "L%d:\n", label);
}

/**
 * 创建一个跳到 label 的 jmp
*/
void register_jump(int label) {
  fprintf(output_file, "\tb\tL%d\n", label);
}

/**
 * 解析函数定义的前置汇编代码
*/
void register_function_preamble(int symbol_table_index) {
  char *name = global_symbol_table[symbol_table_index].name;
  fprintf(output_file,
          "\t.text\n"
          "\t.globl\t%s\n"
          "\t.type\t%s, \%%function\n"
          "%s:\n" "\tpush\t{fp, lr}\n"
          "\tadd\tfp, sp, #4\n"
          "\tsub\tsp, sp, #8\n"
          "\tstr\tr0, [fp, #-8]\n",
            name, name, name);
}

/**
 * 解析函数定义的后置汇编代码
*/
void register_function_postamble(int symbol_table_index) {
  struct SymbolTable t = global_symbol_table[symbol_table_index];
  register_label(t.end_label);
  fputs("\tsub\tsp, fp, #4\n"
        "\tpop\t{fp, pc}\n"
        "\t.align\t2\n"
        , output_file);
}

/**
 * 从 old_primitive_type 转向 new_primitive_type 时扩大在寄存器中的值
 * 返回这个新的值
*/
int register_widen(
  int register_index,
  int old_primitive_type,
  int new_primitive_type) {
    return register_index;
}

/**
 * 给定一个 primitive type，返回其对应的字节数
*/
int register_get_primitive_type_size(int primitive_type) {
  if (primitive_type < PRIMITIVE_NONE || primitive_type > PRIMITIVE_LONG_POINTER)
    error("Bad type in register_get_primitive_type_size()");
  return primitive_size[primitive_type];
}

/**
 * 处理函数调用 function_call
*/
int register_function_call(int register_index, int symbol_table_index) {
  fprintf(output_file, "\tmov\tr0, %s\n", register_list[register_index]);
  fprintf(output_file, "\tbl\t%s\n", global_symbol_table[symbol_table_index].name);
  fprintf(output_file, "\tmov\t%s, r0\n", register_list[register_index]);
  return register_index;
}

/**
 * 处理函数返回 function_return
*/
void register_function_return(int register_index, int symbol_table_index) {
  struct SymbolTable t = global_symbol_table[symbol_table_index];
  char *r = register_list[register_index];
  fprintf(output_file, "\tmov\tr0, %s\n", r);
  register_jump(t.end_label);
}

int register_load_identifier_address(int symbol_table_index) {
  int register_index = allocate_register();
  set_variable_offset(symbol_table_index);
  fprintf(output_file, "\tmov\t%s, r3\n", register_list[register_index]);
  return register_index;
}

int register_dereference_pointer(int register_index, int primitive_type) {
  char *r = register_list[register_index];
  switch (primitive_type) {
    case PRIMITIVE_CHAR_POINTER:
      fprintf(output_file, "\tldrb\t%s, [%s]\n", r, r);
      break;
    case PRIMITIVE_INT_POINTER:
    case PRIMITIVE_LONG_POINTER:
      fprintf(output_file, "\tldr\t%s, [%s]\n", r, r);
      break;
    default:
      error_with_digital("Can't register_dereference_pointer on type:", primitive_type);
  }
  return register_index;
}

int register_store_dereference_pointer(int left_register, int right_register, int primitive_type) {
  char *lr = register_list[left_register];
  char *rr = register_list[right_register];
  switch (primitive_type) {
    case PRIMITIVE_CHAR:
      fprintf(output_file, "\tstrb\t%s, [%s]\n", lr, rr);
      break;
    case PRIMITIVE_INT:
    case PRIMITIVE_LONG:
      fprintf(output_file, "\tstr\t%s, [%s]\n", lr, rr);
      break;
    default:
      error_with_digital("Can't register_store_dereference_pointer on type:", primitive_type);
  }
  return left_register;
}

int register_shift_left_by_constant(int register_index, int value) {
  char *r = register_list[register_index];
  fprintf(output_file, "\tlsl\t%s, %s, #%d\n", r, r, value);
  return register_index;
}

/**
 * 给定一个全局 string 的 label id，把它的地址加载进一个寄存器中
*/
int register_load_global_string(int label) {
  int register_index = allocate_register();
  // fprintf(output_file, "\tleaq\tL%d(\%%rip), %s\n", label, register_list[register_index]);
  return register_index;
}
