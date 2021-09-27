#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define extern_
  #include "data.h"
#undef extern_

#include "definations.h"
#include "scan.h"
#include "parser.h"
#include "interpreter.h"
#include "generator.h"
#include "statement.h"
#include "helper.h"
#include "declaration.h"
#include "symbol_table.h"

static void init() {
  line = 1;
  putback_buffer = '\n';
  global_symbol_table_index = 0;
  output_dump_ast = 0;
  local_symbol_table_index = SYMBOL_TABLE_ENTRIES_NUMBER - 1;
}

static void usage_info(char *info) {
  fprintf(stderr, "Usage: %s [-T] input_file", info);
  exit(1);
}

int main(int argc, char *argv[]) {
  int i;

  init();

  // 扫描命令行输入
  for (i = 1; i < argc; i++) {
    if (*argv[i] != '-') break;
    for (int j = 1; argv[i][j]; j++) {
      switch (argv[i][j]) {
        case 'T': output_dump_ast = 1; break;
        default: usage_info(argv[0]);
      }
    }
  }

  if (i >= argc) usage_info(argv[0]);

  if (!(input_file = fopen(argv[i], "r"))) {
    fprintf(stderr, "Unable to open %s: %s\n", argv[1], strerror(errno));
    exit(1);
  }

  // 用 output_file 来模拟一个被生成的汇编文件
  if (!(output_file = fopen("out.s", "w"))) {
    fprintf(stderr, "Unable to open %s: %s\n", "out.s", strerror(errno));
    exit(1);
  }

  // 确保 print_int 是已经定义的
  add_global_symbol("print_int", PRIMITIVE_INT, STRUCTURAL_FUNCTION, 0, 0, STORAGE_CLASS_GLOBAL);
  add_global_symbol("print_char", PRIMITIVE_VOID, STRUCTURAL_FUNCTION, 0, 0, STORAGE_CLASS_GLOBAL);

  // 扫描文件中的字符串，并将其赋值给 token_from_file 这个全局变量
  scan(&token_from_file);
  generate_preamble_code();
  parse_global_declaration_statement();
  generate_postamble_code();

  // 关闭文件
  fclose(input_file);
  fclose(output_file);

  exit(0);

  return 0;
}
