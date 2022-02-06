#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

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

#define MAX_OBJECT_FILE_NUMBER 100

static void init() {
  output_dump_ast = 0;
  output_assemble_assembly_file = 0;
  output_keep_assembly_file = 0;
  output_link_object_file = 1;
  output_verbose = 0;
  output_dump_symbol_table = 0;
}

static void usage_info(char *info) {
  fprintf(stderr, "Usage: %s [-vcSTM] [-o output file] file [file ...]\n", info);
  fprintf(stderr, "       -c generate object files but don't link them\n");
  fprintf(stderr, "       -S generate assembly files but don't link them\n");
  fprintf(stderr, "       -T dump the AST trees for each input file\n");
  fprintf(stderr, "       -o output file, produce the output file executable file\n");
  fprintf(stderr, "       -v give verbose output of the compilation stages\n");
  fprintf(stderr, "       -M dump the symbol table for each input file\n");
  exit(1);
}

// 改变一个带 '.' 的字符串的后缀，这个被更改的后缀即为给定的参数 suffix
char *modify_string_suffix(char *string, char suffix) {
  char *p;
  char *new_string = strdup(string);
  if (!new_string) return (NULL);
  // 找出 '.' 的位置
  p = strrchr(new_string, '.');
  if (!p) return (NULL);
  // 边界判断
  if (*(++p) == '\0') return (NULL);
  // 更改 suffix
  *(p++) = suffix;
  *p = '\0';
  return (new_string);
}

void clear_all_static_symbol() {
  struct SymbolTable *current_symbol_table, *prev_symbol_table = NULL;

  // 遍历 global symbol table，寻找 static symbol
  for (
    current_symbol_table = global_head;
    current_symbol_table;
    current_symbol_table = current_symbol_table->next
  ) {
    // 如果找到了
    if (current_symbol_table->storage_class == STORAGE_CLASS_STATIC) {
      // 正常情况: 跳过 static symbol
      if (prev_symbol_table)
        prev_symbol_table->next = current_symbol_table->next;
      else
        global_head->next = current_symbol_table->next;

      // 如果 static symbol 出现在结尾: 跳过 static symbol
      if (current_symbol_table == global_tail) {
        if (prev_symbol_table) global_tail = prev_symbol_table;
        else global_tail = global_head;
      }
    }
  }

  // 在指向下一个之前 prev 应该指向当前 node
  prev_symbol_table = current_symbol_table;
}

// 编译成汇编代码
static char *do_compile(char *filename) {
  char cmd[TEXT_LENGTH];

  global_output_filename = modify_string_suffix(filename, 's');
  if (!global_output_filename) {
    fprintf(stderr, "Error: %s has no suffix, try .zc on the end\n", filename);
    exit(1);
  }

  // 先生成一个预处理器的命令行
  // INCLUDE_DIRECTORY 在 Makefile 里面找到
  snprintf(cmd, TEXT_LENGTH, "%s %s %s", CPP_CMD, INCDIR, filename);
  if (!(input_file = popen(cmd, "r"))) {
    fprintf(stderr, "Unable to open %s: %s\n", filename, strerror(errno));
    exit(1);
  }
  global_input_filename = filename;

  // 用 output_file 来模拟一个被生成的汇编文件
  if (!(output_file = fopen(global_output_filename, "w"))) {
    fprintf(stderr, "Unable to open %s: %s\n", global_output_filename, strerror(errno));
    exit(1);
  }

  line = 1;
  start_line = 1;
  putback_buffer = '\n';
  clear_all_symbol_tables();

  if (output_verbose)
    printf("compiling %s\n", filename);

  // 扫描文件中的字符串，并将其赋值给 token_from_file 这个全局变量
  scan(&token_from_file);
  look_ahead_token.token = 0;
  generate_preamble_code();
  parse_global_declaration();
  generate_postamble_code();

  // 关闭文件
  fclose(input_file);
  fclose(output_file);

  if (output_dump_symbol_table) {
    printf("Symbols for %s\n", filename);
    dump_symbol_table();
    fprintf(stdout, "\n\n");
  }

  clear_all_static_symbol();

  return (global_output_filename);
}

char *do_assemble(char *filename) {
  char cmd[TEXT_LENGTH];
  int error;

  char *output_filename = modify_string_suffix(filename, 'o');
  if (!output_filename) {
    fprintf(stderr, "Error: %s has no suffix, try .s on the end\n", filename);
    exit(1);
  }

  snprintf(cmd, TEXT_LENGTH, "%s %s %s", AS_CMD, output_filename, filename);
  if (output_verbose) printf("%s\n", cmd);
  error = system(cmd);
  if (error) {
    fprintf(stderr, "Assembly of %s failed\n", filename);
    exit(1);
  }
  return (output_filename);
}

// link 文件
void do_link(char *output_filename, char **object_file_list) {
  int count, size = TEXT_LENGTH;
  char cmd[TEXT_LENGTH], *p;
  int error;

  p = cmd;
  count = snprintf(p, size, "%s %s ", LD_CMD, output_filename);
  p += count;
  size -= count;

  while (*object_file_list) {
    count = snprintf(p, size, "%s ", *object_file_list);
    p += count;
    size -= count;
    object_file_list++;
  }

  if (output_verbose) printf("%s\n", cmd);
  error = system(cmd);
  if (error) {
    fprintf(stderr, "Linking failed\n");
    exit(1);
  }
}

void do_unlink(char *filename) {
  unlink(filename);
}



int main(int argc, char **argv) {
  char *output_filename = A_OUT;
  char *assembly_file, *object_file;
  char *object_file_list[MAX_OBJECT_FILE_NUMBER];
  int i, j, object_file_count = 0;

  init();

  // 扫描命令行输入
  for (i = 1; i < argc; i++) {
    if (*argv[i] != '-') break;
    for (j = 1; (*argv[i] == '-') && argv[i][j]; j++) {
      switch (argv[i][j]) {
        case 'T': output_dump_ast = 1; break;
        case 'o': output_filename = argv[++i]; break;
        case 'c':
          output_assemble_assembly_file = 1;
          output_keep_assembly_file = 0;
          output_link_object_file = 0;
          break;
        case 'S':
          output_assemble_assembly_file = 0;
          output_keep_assembly_file = 1;
          output_link_object_file = 0;
          break;
        case 'v': output_verbose = 1; break;
        case 'M': output_dump_symbol_table = 1; break;
        default: usage_info(argv[0]);
      }
    }
  }

  if (i >= argc) usage_info(argv[0]);

  // 轮流编译文件
  while (i < argc) {
    assembly_file = do_compile(argv[i]);

    if (output_link_object_file || output_assemble_assembly_file) {
      object_file = do_assemble(assembly_file);
      if (object_file_count == (MAX_OBJECT_FILE_NUMBER - 2)) {
        fprintf(stderr, "Too many object files for the compiler to handle\n");
        exit(1);
      }
      object_file_list[object_file_count++] = object_file;
      object_file_list[object_file_count] = NULL;
    }

    if (!output_keep_assembly_file) do_unlink(assembly_file);
    i++;
  }

  if (output_link_object_file) {
    do_link(output_filename, object_file_list);

    if (!output_assemble_assembly_file)
      for(i = 0; object_file_list[i]; i++)
        do_unlink(object_file_list[i]);
  }

  return (0);
}
