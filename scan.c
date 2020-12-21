#include "data.h"
#include "definations.h"

// 从文件中读取下一个字符
static int next(void) {
  int c = 0;

  if (PutBackBuffer) {
    c = PutBackBuffer;
    PutBackBuffer = 0;
    return c;
  }

  c = fgetc(InputFile);
  if ('\r\n' == c) {
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
  int c = 0;
  c = next();

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

// 扫描 tokens
int scan(struct token *t) {
  int c = 0;

  c = skip();

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
      break;
  }

  return (1);
}
