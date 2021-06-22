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

// 声明 token 字符串数组
const char *token_string[] = { "+", "-", "*", "/", "integer_literal" };

static void init() {
  line = 1;
  putback_buffer = '\n';
}

static void usage_info(char *info) {
  fprintf(stderr, "Usage: %s input_file", info);
  exit(1);
}

int main(int argc, char *argv[]) {
  struct ASTNode *tree;
  int result;

  if (argc != 2) usage_info(argv[0]);

  init();

  if (!(input_file = fopen(argv[1], "r"))) {
    fprintf(stderr, "Unable to open %s: %s\n", argv[1], strerror(errno));
    exit(1);
  }

  // 用 output_file 来模拟一个被生成的汇编文件
  if (!(output_file = fopen("out.s", "w"))) {
    fprintf(stderr, "Unable to open %s: %s\n", "out.s", strerror(errno));
    exit(1);
  }

  // 扫描文件中的字符串，并将其赋值给 token_from_file 这个全局变量
  scan(&token_from_file);
  // // 开始在解析的过程中，把字符串转换成 ast
  // tree = converse_token_2_ast(0);
  // // 再将这颗树解析，并输出结果
  // result = interpret_ast(tree);
  // printf("Expression result = %d\n", result);

  // // 生成汇编代码
  // generate_code(tree);

  // 生成汇编代码
  // 这里主要是测试有 print 的语句的情况
  generate_preamble_code();
  parse_compound_statement();
  generate_postamble_code();

  // 关闭文件
  fclose(input_file);
  fclose(output_file);

  exit(0);

  return 0;
}
