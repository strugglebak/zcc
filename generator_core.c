#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "data.h"
#include "definations.h"


#ifdef _linux
  #define CALL_PRINTF "\tcall	printf\n"
  #define GLOBAL_MAIN "\t.globl\tmain\n"
  #define MAIN "main:\n"
#else
  #define CALL_PRINTF "\tcall	_printf\n"
  #define GLOBAL_MAIN "\t.globl\t_main\n"
  #define MAIN "_main:\n"
#endif

static int free_registers[4] = { 1 };
static char *register_list[4] = { "%r8", "%r9", "%r10", "%r11" };
static char *lower_8_bits_register_list[4] = { "%r8b", "%r9b", "%r10b", "%r11b" };


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
  fputs(
    "\t.text\n"
    ".LC0:\n"
    "\t.string\t\"%d\\n\"\n"
    "register_print:\n"
    "\tpushq\t%rbp\n"
    "\tmovq\t%rsp, %rbp\n"
    "\tsubq\t$16, %rsp\n"
    "\tmovl\t%edi, -4(%rbp)\n"
    "\tmovl\t-4(%rbp), %eax\n"
    "\tmovl\t%eax, %esi\n"
    "\tleaq	.LC0(%rip), %rdi\n"
    "\tmovl	$0, %eax\n"
    // MacOS & iOS 不用 PLT
    // https://stackoverflow.com/questions/59817831/unsupported-symbol-modifier-in-branch-relocation-call-printfplt
    // "\tcall	_printf@PLT\n"
    CALL_PRINTF
    "\tnop\n"
    "\tleave\n"
    "\tret\n"
    "\n"
    GLOBAL_MAIN
    // 如果是要编译成 Mach-O x86-64 的汇编，则不需要下面的这个指令，因为这个指令为一个调试用的指令，通常用来调用桢信息的
    // 详情见
    // https://stackoverflow.com/questions/19720084/what-is-the-difference-between-assembly-on-mac-and-assembly-on-linux/19725269#19725269
    // "\t.type\tmain, @function\n"
    MAIN
    "\tpushq\t%rbp\n"
    "\tmovq	%rsp, %rbp\n",
    output_file
  );
}

/**
 * 汇编后置代码，写入到 output_file 中
*/
void register_postamble() {
  fputs(
    "\tmovl	$0, %eax\n"
    "\tpopq	%rbp\n"
    "\tret\n",
    output_file
  );
}

/**
 * 把 value 放入某个寄存器中，并返回该寄存器的下标
*/
int register_load_interger_literal(int value) {
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
 * 打印指定的寄存器
*/
void register_print(int register_index) {
  fprintf(output_file, "\tmovq\t%s, %%rdi\n", register_list[register_index]);
  fprintf(output_file, "\tcall\tregister_print\n");
  clear_register(register_index);
}

/**
 * 将一个变量的值保存到寄存器中
*/
int register_load_value_from_variable(char *identifier) {
  int register_index = allocate_register();
  fprintf(output_file, "\tmovq\t%s(\%%rip), %s\n", identifier, register_list[register_index]);
  return register_index;
}

/**
 * 将寄存器中的值保存到一个变量中
*/
int register_store_value_2_variable(int register_index, char *identifier) {
  fprintf(output_file, "\tmovq\t%s, %s(\%%rip)\n", register_list[register_index], identifier);
  return register_index;
}

/**
 * 创建全局变量
*/
void register_generate_global_symbol(char *symbol_string) {
  fprintf(output_file, "\t.comm\t%s,8,8\n", symbol_string);
}

/**
 * 比较 =
*/
int register_compare_equal(int register_left, int register_right) {
  return compare_register(register_left, register_right, "sete");
}

/**
 * 比较 !=
*/
int register_compare_not_equal(int register_left, int register_right) {
  return compare_register(register_left, register_right, "setne");
}

/**
 * 比较 <
*/
int register_compare_less_than(int register_left, int register_right) {
  return compare_register(register_left, register_right, "setl");
}

/**
 * 比较 >
*/
int register_compare_greater_than(int register_left, int register_right) {
  return compare_register(register_left, register_right, "setg");
}

/**
 * 比较 <=
*/
int register_compare_less_equal(int register_left, int register_right) {
  return compare_register(register_left, register_right, "setle");
}

/**
 * 比较 >=
*/
int register_compare_greater_equal(int register_left, int register_right) {
  return compare_register(register_left, register_right, "setge");
}
