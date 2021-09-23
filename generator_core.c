#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "data.h"
#include "definations.h"
#include "helper.h"
#include "generator_core.h"

enum {
  NO_SECTION_FLAG,
  TEXT_SECTION_FLAG,
  DATA_SECTION_FLAG,
} current_section_flag = NO_SECTION_FLAG;

static int free_registers[4] = { 1 };
static char *register_list[4] = { "%r8", "%r9", "%r10", "%r11" }; // 64 位寄存器
static char *lower_8_bits_register_list[4] = { "%r8b", "%r9b", "%r10b", "%r11b" }; // 低 8 位寄存器
static char *lower_32_bits_register_list[4] = { "%r8d", "%r9d", "%r10d", "%r11d" }; // 低 32 位寄存器

static char *compare_list[] =
  { "sete", "setne", "setl", "setg", "setle", "setge" };

static char *inverted_compare_list[] = { "jne", "je", "jge", "jle", "jg", "jl" };

// none/void/char/int/long
static int primitive_size[] = { 0, 0, 1, 4, 8, 8, 8, 8 };

// 相对于栈基指针的本地变量的位置
static int local_offset;
static int stack_offset;

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

  fprintf(stderr, "Out of registers\n");
  exit(1);
}

/**
 * 清空某个寄存器
*/
static void clear_register(int index) {
  if (free_registers[index]) {
    fprintf(stderr, "Error trying to clear registers\n");
    exit(1);
  }
  free_registers[index] = 1;
}

/**
 * 比较两个寄存器值的大小
 * 使用 cmpq 比较了大小之后，需要将一个 8 位的寄存器置位
*/
static int compare_register(int left_register, int right_register, char *set_instruction) {
  fprintf(output_file, "\tcmpq\t%s, %s\n", register_list[right_register], register_list[left_register]);
  fprintf(output_file, "\t%s\t%s\n", set_instruction, lower_8_bits_register_list[right_register]);
  fprintf(output_file, "\tandq\t$255,%s\n", register_list[right_register]);
  clear_register(left_register);
  return right_register;
}

/**
 * 汇编前置代码，写入到 output_file 中
*/
void register_preamble() {
  clear_all_registers();
}

/**
 * 汇编后置代码，写入到 output_file 中
*/
void register_postamble() {
}

/**
 * 把 value 放入某个寄存器中，并返回该寄存器的下标
*/
int register_load_interger_literal(int value, int primitive_type) {
  int register_index = allocate_register();
  fprintf(output_file, "\tmovq\t$%d, %s\n", value, register_list[register_index]);
  return register_index;
}

/**
 * 两个寄存器相加，并将其放入其中一个寄存器中
*/
int register_plus(int left_register, int right_register) {
  fprintf(output_file, "\taddq\t%s, %s\n", register_list[left_register], register_list[right_register]);
  clear_register(left_register);
  return right_register;
}

/**
 * 两个寄存器相减，并将其放入其中一个寄存器中
*/
int register_minus(int left_register, int right_register) {
  fprintf(output_file, "\tsubq\t%s, %s\n", register_list[right_register], register_list[left_register]);
  clear_register(right_register);
  return left_register;
}

/**
 * 两个寄存器相乘，并将其放入其中一个寄存器中
*/
int register_multiply(int left_register, int right_register) {
  fprintf(output_file, "\timulq\t%s, %s\n", register_list[left_register], register_list[right_register]);
  clear_register(left_register);
  return right_register;
}

/**
 * 两个寄存器相除，这里处理比较麻烦
 * 先把值放入 rax 寄存器
 * 进行除法运算后，再从 rax 寄存器中把结果拿出来，放入 left_register
*/
int register_divide(int left_register, int right_register) {
  fprintf(output_file, "\tmovq\t%s, %%rax\n", register_list[left_register]);
  fprintf(output_file, "\tcqo\n");
  fprintf(output_file, "\tidivq\t%s\n", register_list[right_register]);
  fprintf(output_file, "\tmovq\t%%rax, %s\n", register_list[left_register]);
  clear_register(right_register);
  return left_register;
}

/**
 * 将一个变量的值保存到寄存器中
*/
int register_load_value_from_variable(int symbol_table_index, int operation) {
  int register_index = allocate_register();
  struct SymbolTable t = symbol_table[symbol_table_index];
  char *r = register_list[register_index];
  switch (t.primitive_type) {
    case PRIMITIVE_CHAR:
      if (operation == AST_PRE_INCREASE)
        fprintf(output_file, "\tincb\t%s(\%%rip)\n", t.name);
      if (operation == AST_PRE_DECREASE)
        fprintf(output_file, "\tdecb\t%s(\%%rip)\n", t.name);

      fprintf(output_file, "\tmovzbq\t%s(\%%rip), %s\n", t.name, r);

      if (operation == AST_POST_INCREASE)
        fprintf(output_file, "\tincb\t%s(\%%rip)\n", t.name);
      if (operation == AST_POST_DECREASE)
        fprintf(output_file, "\tdecb\t%s(\%%rip)\n", t.name);
      break;
    case PRIMITIVE_INT:
      if (operation == AST_PRE_INCREASE)
        fprintf(output_file, "\tincl\t%s(\%%rip)\n", t.name);
      if (operation == AST_PRE_DECREASE)
        fprintf(output_file, "\tdecl\t%s(\%%rip)\n", t.name);

      fprintf(output_file, "\tmovslq\t%s(\%%rip), %s\n", t.name, r);

      if (operation == AST_POST_INCREASE)
        fprintf(output_file, "\tincl\t%s(\%%rip)\n", t.name);
      if (operation == AST_POST_DECREASE)
        fprintf(output_file, "\tdecl\t%s(\%%rip)\n", t.name);
      break;
    case PRIMITIVE_LONG:
    case PRIMITIVE_CHAR_POINTER:
    case PRIMITIVE_INT_POINTER:
    case PRIMITIVE_LONG_POINTER:
      if (operation == AST_PRE_INCREASE)
        fprintf(output_file, "\tincq\t%s(\%%rip)\n", t.name);
      if (operation == AST_PRE_DECREASE)
        fprintf(output_file, "\tdecq\t%s(\%%rip)\n", t.name);

      fprintf(output_file, "\tmovq\t%s(\%%rip), %s\n", t.name, r);

      if (operation == AST_POST_INCREASE)
        fprintf(output_file, "\tincq\t%s(\%%rip)\n", t.name);
      if (operation == AST_POST_DECREASE)
        fprintf(output_file, "\tdecq\t%s(\%%rip)\n", t.name);
      break;
    default:
      error_with_digital("Bad type in register_load_value_from_variable:", t.primitive_type);
  }
  return register_index;
}

/**
 * 将一个局部变量的值保存到寄存器中
*/
int register_load_local_value_from_variable(int symbol_table_index, int operation) {
  int register_index = allocate_register();
  struct SymbolTable t = symbol_table[symbol_table_index];
  char *r = register_list[register_index];
  switch (t.primitive_type) {
    case PRIMITIVE_CHAR:
      if (operation == AST_PRE_INCREASE)
        fprintf(output_file, "\tincb\t%d(\%%rbp)\n", t.position);
      if (operation == AST_PRE_DECREASE)
        fprintf(output_file, "\tdecb\t%d(\%%rbp)\n", t.position);

      fprintf(output_file, "\tmovzbq\t%d(\%%rbp), %s\n", t.position, r);

      if (operation == AST_POST_INCREASE)
        fprintf(output_file, "\tincb\t%d(\%%rbp)\n", t.position);
      if (operation == AST_POST_DECREASE)
        fprintf(output_file, "\tdecb\t%d(\%%rbp)\n", t.position);
      break;
    case PRIMITIVE_INT:
      if (operation == AST_PRE_INCREASE)
        fprintf(output_file, "\tincl\t%d(\%%rbp)\n", t.position);
      if (operation == AST_PRE_DECREASE)
        fprintf(output_file, "\tdecl\t%d(\%%rbp)\n", t.position);

      fprintf(output_file, "\tmovslq\t%d(\%%rbp), %s\n", t.position, r);

      if (operation == AST_POST_INCREASE)
        fprintf(output_file, "\tincl\t%d(\%%rbp)\n", t.position);
      if (operation == AST_POST_DECREASE)
        fprintf(output_file, "\tdecl\t%d(\%%rbp)\n", t.position);
      break;
    case PRIMITIVE_LONG:
    case PRIMITIVE_CHAR_POINTER:
    case PRIMITIVE_INT_POINTER:
    case PRIMITIVE_LONG_POINTER:
      if (operation == AST_PRE_INCREASE)
        fprintf(output_file, "\tincq\t%d(\%%rbp)\n", t.position);
      if (operation == AST_PRE_DECREASE)
        fprintf(output_file, "\tdecq\t%d(\%%rbp)\n", t.position);

      fprintf(output_file, "\tmovq\t%d(\%%rbp), %s\n", t.position, r);

      if (operation == AST_POST_INCREASE)
        fprintf(output_file, "\tincq\t%d(\%%rbp)\n", t.position);
      if (operation == AST_POST_DECREASE)
        fprintf(output_file, "\tdecq\t%d(\%%rbp)\n", t.position);
      break;
    default:
      error_with_digital("Bad type in register_load_local_value_from_variable:", t.primitive_type);
  }
  return register_index;
}

/**
 * 将寄存器中的值保存到一个变量中
*/
int register_store_value_2_variable(int register_index, int symbol_table_index) {
  struct SymbolTable t = symbol_table[symbol_table_index];
  switch (t.primitive_type) {
    case PRIMITIVE_CHAR:
      fprintf(output_file, "\tmovb\t%s, %s(\%%rip)\n",
       lower_8_bits_register_list[register_index],
       t.name);
      break;
    case PRIMITIVE_INT:
      fprintf(output_file, "\tmovl\t%s, %s(\%%rip)\n",
       lower_32_bits_register_list[register_index],
       t.name);
      break;
    case PRIMITIVE_LONG:
    case PRIMITIVE_CHAR_POINTER:
    case PRIMITIVE_INT_POINTER:
    case PRIMITIVE_LONG_POINTER:
      fprintf(output_file, "\tmovq\t%s, %s(\%%rip)\n",
      register_list[register_index],
      t.name);
      break;
    default:
      error_with_digital("Bad type in register_store_value_2_variable:", t.primitive_type);
  }
  return register_index;
}

/**
 * 将寄存器中的值保存到一个局部变量中
*/
int register_store_local_value_2_variable(int register_index, int symbol_table_index) {
  struct SymbolTable t = symbol_table[symbol_table_index];
  switch (t.primitive_type) {
    case PRIMITIVE_CHAR:
      fprintf(output_file, "\tmovb\t%s, %d(\%%rbp)\n",
       lower_8_bits_register_list[register_index],
       t.position);
      break;
    case PRIMITIVE_INT:
      fprintf(output_file, "\tmovl\t%s, %d(\%%rbp)\n",
       lower_32_bits_register_list[register_index],
       t.position);
      break;
    case PRIMITIVE_LONG:
    case PRIMITIVE_CHAR_POINTER:
    case PRIMITIVE_INT_POINTER:
    case PRIMITIVE_LONG_POINTER:
      fprintf(output_file, "\tmovq\t%s, %d(\%%rbp)\n",
      register_list[register_index],
      t.position);
      break;
    default:
      error_with_digital("Bad type in register_store_local_value_2_variable:", t.primitive_type);
  }
  return register_index;
}

/**
 * 创建全局变量
*/
void register_generate_global_symbol(int symbol_table_index) {
  struct SymbolTable t = symbol_table[symbol_table_index];
  int primitive_type_size = register_get_primitive_type_size(t.primitive_type);
  if (t.structural_type == STRUCTURAL_FUNCTION) return;
  register_data_section_flag();
  fprintf(output_file, "\t.globl\t%s\n", t.name);
  fprintf(output_file, "%s:", t.name);
  // 支持类似 char a[10]; 这样的写法，size 就是 10
  for (int i = 0; i < t.size; i++) {
    switch(primitive_type_size) {
      case 1: fprintf(output_file, "\t.byte\t0\n"); break;
      case 4: fprintf(output_file, "\t.long\t0\n"); break;
      case 8: fprintf(output_file, "\t.quad\t0\n"); break;
      default: error_with_digital("Unknown typesize in register_generate_global_symbol: ", primitive_type_size);
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

  fprintf(output_file, "\tcmpq\t%s, %s\n",
    register_list[register_right],
    register_list[register_left]);

  fprintf(output_file, "\t%s\t%s\n",
    compare_list[ast_operation - AST_COMPARE_EQUALS],
    lower_8_bits_register_list[register_right]);

  fprintf(output_file, "\tmovzbq\t%s, %s\n",
    lower_8_bits_register_list[register_right],
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

  fprintf(output_file, "\tcmpq\t%s, %s\n",
    register_list[register_right],
    register_list[register_left]);
  fprintf(output_file, "\t%s\tL%d\n",
    inverted_compare_list[ast_operation - AST_COMPARE_EQUALS],
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
  fprintf(output_file, "\tjmp\tL%d\n", label);
}

/**
 * 解析函数定义的前置汇编代码
*/
void register_function_preamble(int symbol_table_index) {
  char *name = symbol_table[symbol_table_index].name;
  register_text_section_flag();

  // 将栈指针对齐为 16 的倍数
  stack_offset = (local_offset+15) & (~15);
  fprintf(output_file,
            "\t.globl\t%s\n"
            "\t.type\t%s, @function\n"
            "%s:\n"
            "\tpushq\t%%rbp\n"
            "\tmovq\t%%rsp, %%rbp\n"
            "\taddq\t$%d,%%rsp\n",
            name, name, name, -stack_offset);
}

/**
 * 解析函数定义的后置汇编代码
*/
void register_function_postamble(int symbol_table_index) {
  struct SymbolTable t = symbol_table[symbol_table_index];
  register_label(t.end_label);
  fprintf(output_file, "\taddq\t$%d,%%rsp\n", stack_offset);
  fputs("\tpopq\t%rbp\n"
        "\tret\n", output_file);
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
  int out_register_index = allocate_register();
  fprintf(output_file, "\tmovq\t%s, %%rdi\n", register_list[register_index]);
  fprintf(output_file, "\tcall\t%s\n", symbol_table[symbol_table_index].name);
  fprintf(output_file, "\tmovq\t%%rax, %s\n", register_list[out_register_index]);
  clear_register(register_index);
  return out_register_index;
}

/**
 * 处理函数返回 function_return
*/
void register_function_return(int register_index, int symbol_table_index) {
  struct SymbolTable t = symbol_table[symbol_table_index];
  char *r = register_list[register_index];
  switch (t.primitive_type) {
    case PRIMITIVE_CHAR:
      fprintf(output_file, "\tmovzbl\t%s, %%eax\n", lower_8_bits_register_list[register_index]);
      break;
    case PRIMITIVE_INT:
      fprintf(output_file, "\tmovl\t%s, %%eax\n", lower_32_bits_register_list[register_index]);
      break;
    case PRIMITIVE_LONG:
      fprintf(output_file, "\tmovq\t%s, %%rax\n", register_list[register_index]);
      break;
    default:
      error_with_digital("Bad function type in register_function_return:", t.primitive_type);
  }
  register_jump(t.end_label);
}

int register_load_identifier_address(int symbol_table_index) {
  int register_index = allocate_register();
  struct SymbolTable t = symbol_table[symbol_table_index];
  char *r = register_list[register_index];
  if (t.storage_class == CENTRAL_LOCAL)
    fprintf(output_file, "\tleaq\t%d(%%rbp), %s\n", t.position, r);
  else
    fprintf(output_file, "\tleaq\t%s(%%rip), %s\n", t.name, r);
  return register_index;
}

int register_dereference_pointer(int register_index, int primitive_type) {
  char *r = register_list[register_index];
  switch (primitive_type) {
    case PRIMITIVE_CHAR_POINTER:
      fprintf(output_file, "\tmovzbq\t(%s), %s\n", r, r);
      break;
    case PRIMITIVE_INT_POINTER:
      fprintf(output_file, "\tmovslq\t(%s), %s\n", r, r);
      break;
    case PRIMITIVE_LONG_POINTER:
      fprintf(output_file, "\tmovq\t(%s), %s\n", r, r);
      break;
    default:
      error_with_digital("Can't register_dereference_pointer on type:", primitive_type);
  }
  return register_index;
}

int register_store_dereference_pointer(int left_register, int right_register, int primitive_type) {
  switch (primitive_type) {
    case PRIMITIVE_CHAR:
      fprintf(output_file, "\tmovb\t%s, (%s)\n",
        lower_8_bits_register_list[left_register],
        register_list[right_register]);
      break;
    case PRIMITIVE_INT:
    case PRIMITIVE_LONG:
      fprintf(output_file, "\tmovq\t%s, (%s)\n",
        register_list[left_register],
        register_list[right_register]);
      break;
    default:
      error_with_digital("Can't register_store_dereference_pointer on type:", primitive_type);
  }
  return left_register;
}

int register_shift_left_by_constant(int register_index, int value) {
  char *r = register_list[register_index];
  fprintf(output_file, "\tsalq\t$%d, %s\n", value, r);
  return register_index;
}

/**
 * 给定一个全局 string 的 label id，把它的地址加载进一个寄存器中
*/
int register_load_global_string(int label) {
  int register_index = allocate_register();
  fprintf(output_file, "\tleaq\tL%d(\%%rip), %s\n", label, register_list[register_index]);
  return register_index;
}

int register_and(int left_register, int right_register) {
  fprintf(output_file, "\tandq\t%s, %s\n",
    register_list[left_register],
    register_list[right_register]);
  clear_register(left_register);
  return right_register;
}

int register_or(int left_register, int right_register) {
  fprintf(output_file, "\torq\t%s, %s\n",
    register_list[left_register],
    register_list[right_register]);
  clear_register(left_register);
  return right_register;
}

int register_xor(int left_register, int right_register) {
  fprintf(output_file, "\txorq\t%s, %s\n",
    register_list[left_register],
    register_list[right_register]);
  clear_register(left_register);
  return right_register;
}

int register_negate(int register_index) {
  fprintf(output_file, "\tnegq\t%s\n",
    register_list[register_index]);
  return register_index;
}

int register_invert(int register_index) {
  fprintf(output_file, "\tnotq\t%s\n",
    register_list[register_index]);
  return register_index;
}

int register_shift_left(int left_register, int right_register) {
  fprintf(output_file, "\tmovb\t%s, %%cl\n", lower_8_bits_register_list[right_register]);
  fprintf(output_file, "\tshlq\t%%cl, %s\n", register_list[left_register]);
  clear_register(right_register);
  return left_register;
}

int register_shift_right(int left_register, int right_register) {
  fprintf(output_file, "\tmovb\t%s, %%cl\n", lower_8_bits_register_list[right_register]);
  fprintf(output_file, "\tshrq\t%%cl, %s\n", register_list[left_register]);
  clear_register(right_register);
  return left_register;
}

int register_logic_not(int register_index) {
  fprintf(output_file, "\ttest\t%s, %s\n", register_list[register_index], register_list[register_index]);
  fprintf(output_file, "\tsete\t%s\n", lower_8_bits_register_list[register_index]);
  fprintf(output_file, "\tmovzbq\t%s, %s\n", lower_8_bits_register_list[register_index], register_list[register_index]);
  return register_index;
}

int register_to_be_boolean(int register_index, int operaion, int label) {
  fprintf(output_file, "\ttest\t%s, %s\n", register_list[register_index], register_list[register_index]);
  if (operaion == AST_IF || operaion == AST_WHILE)
    fprintf(output_file, "\tje\tL%d\n", label);
  else {
    fprintf(output_file, "\tsetnz\t%s\n", lower_8_bits_register_list[register_index]);
    fprintf(output_file, "\tmovzbq\t%s, %s\n", lower_8_bits_register_list[register_index], register_list[register_index]);
  }
  return register_index;
}

void register_reset_local_variables() {
  local_offset = 0;
}

int register_get_local_offset(int primitive_type, int is_parameter) {
  int primitive_type_size = register_get_primitive_type_size(primitive_type);
  // 栈上位置至少间隔为 4 byte
  local_offset += primitive_type_size > 4 ? primitive_type_size : 4;
  return -local_offset;
}

void register_text_section_flag() {
  if (current_section_flag == TEXT_SECTION_FLAG) return;
  fputs("\t.text\n", output_file);
  current_section_flag = TEXT_SECTION_FLAG;
}

void register_data_section_flag() {
  if (current_section_flag == DATA_SECTION_FLAG) return;
  fputs("\t.data\n", output_file);
  current_section_flag = DATA_SECTION_FLAG;
}
