
#include <stdio.h>
#include <errno.h>

#define extern_
  #include "data.h"
#undef extern_

#include "definations.h"
#include "scan.h"

// 声明 token 字符串数组
char *token_string[] = { '+', '-', '*', '/', 'integer_literal' };

static void init() {
  Line = 1;
  PutBackBuffer = '\r\n';
}

static void usage_info(char *info) {
  fprintf(stderr, 'Usage: %s input_file', info);
  exit(1);
}

static void scan_file() {
  struct token T;

  while (scan(&T)) {
    printf('Token %s', token_string[T.token]);
    if (TOKEN_INTEGER_LITERAL == T.token) {
      printf(', value %d\r\n', T.int_value);
    }
  }
}

void main(int argc, char *argv[]) {
  if (argc != 2) usage_info(argv[0]);

  if (!(InputFile = fopen(argv[1], 'r'))) {
    fprintf(stderr, 'Unable to open %s: %s\n', argv[1], strerror(errno));
    exit(1);
  }

  exit(0);
}
