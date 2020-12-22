#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "data.h"
#include "definations.h"
#include "scan.h"

// 从文件中读取下一个字符
static int next(void) {
  char c;

  if (PutBackBuffer) {
    c = PutBackBuffer;
    PutBackBuffer = 0;
    return c;
  }

  c = fgetc(InputFile);
  if ('\n' == c) {
    Line ++;
  }

  return c;
}

// 把一个不需要的字符放回去
static void put_back(int c) {
  PutBackBuffer = c;
}

// 白名单，遇到如下的字符就跳过
static int skip(void) {
  char c = next();

  while (
    ' ' == c ||
    '\t' == c ||
    '\n' == c ||
    '\t' == c ||
    '\r' == c ||
    '\f' == c
  ) {
    c = next();
  }

  return (c);
}

static int get_the_position_of_the_charater(char *s, int c) {
  char *p;

  p = strchr(s, c);
  // s = "0123456789"
  // s 这里是字符串的首地址，p 是返回这个字符在这个字符串的位置的地址
  // 相减就是对应的值
  return (p ? p - s : -1);
}

// 从输入的 file 中扫描并返回一个 integer 字符
static int scan_integer(char c) {
  int k, value = 0;
  while ((k = get_the_position_of_the_charater("0123456789", c) >= 0)) {
    value = value * 10 + k;
    c = next();
  }

  put_back(c);
  return value;
}

// 扫描 tokens
int scan(struct token *t) {
  // 去掉不需要的字符
  char c = skip();

  switch (c) {
    case EOF:
      return (0);
    case '+':
      t->token = TOKEN_PLUS;
      break;
    case '-':
      t->token = TOKEN_MINUS;
      break;
    case '*':
      t->token = TOKEN_MULTIPLY;
      break;
    case '/':
      t->token = TOKEN_DIVIDE;
      break;
    default:
      if (isdigit(c)) {
        t->int_value = scan_integer(c);
        t->token = TOKEN_INTEGER_LITERAL;
      }
      printf("Unrecognised character %c on line %d\n", c, Line);
      exit(1);
      break;
  }

  return (1);
}
