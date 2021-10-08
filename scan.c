#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "data.h"
#include "definations.h"
#include "scan.h"
#include "helper.h"

static struct Token *rejected_token = NULL;

void reject_token(struct Token *t) {
  if (rejected_token)
    error("Can't reject token twice");
  rejected_token = t;
}

// 从文件中读取下一个字符
static int next(void) {
  char c;

  // 由于上一次读取到了不是数字的字符，所以这里相当于一个 buffer，直接返回上一次读取后的值即可
  if (putback_buffer) {
    c = putback_buffer;
    putback_buffer = 0;
    return c;
  }

  // 返回了上一次读取后的值之后，再次从文件刚刚的位置的下一位读取
  c = fgetc(input_file);
  if ('\n' == c) {
    line ++;
  }

  return c;
}

// 把一个不需要的字符放回去
static void put_back(int c) {
  putback_buffer = c;
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
    // 遇到以上的字符就继续读取
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
  while (((k = get_the_position_of_the_charater("0123456789", c)) >= 0)) {
    value = value * 10 + k;
    // 如果是数字，继续扫描
    c = next();
  }

  // 如果不是数字，则将读取到的字符放到 put buffer 中
  // 准备下一次读取
  // put buffer 在这里可以理解为一个读取完一个 interger 之后的状态
  put_back(c);
  return value;
}

// 扫描标识符，并将其存入 buffer 中，最终返回是这个标识符的长度
// 这个标识符是类似于 printf 之类的函数名称或者其他的变量
static int scan_identifier(int c, char *buffer, int limit_length) {
  int length = 0;

  // 如果是 字母 | 数字 | _
  while(isalpha(c) || isdigit(c) || '_' == c) {
    if (length >= limit_length - 1) {
      error("identifier too long on line");
    }
    buffer[length ++] = c;
    c = next();
  }

  // 跳出循环时，最后一个通过 next 得到的 c 是无效的，所以这里需要 put_back
  put_back(c);

  // 在最后要加结尾符
  buffer[length] = '\0';

  return length;
}

// 扫描单引号中的字符
// 转义部分字符，这里暂时不考虑 8 进制或者其他类型的字符
static int escape_character() {
  int c = next();
  // 处理类似于 \r\n 这种特殊字符
  // 符号 '\'
  if (c == '\\') {
    // 符号 'r'、'n' 等
    switch (c = next()) {
      case 'a': return '\a';
      case 'b': return '\b';
      case 'f': return '\f';
      case 'n': return '\n';
      case 'r': return '\r';
      case 't': return '\t';
      case 'v': return '\v';
      case '\\': return '\\';
      case '"': return '"' ;
      case '\'': return '\'';
      default:
        error_with_character("unknown escape sequence", c);
    }
  }
  return c;
}

// 扫描 sting，并存入 text_buffer 中
// 返回 string 的长度
static int scan_string(char *buffer) {
  int c;
  for (int i = 0; i < TEXT_LENGTH-1; i++) {
    if ((c = escape_character()) == '"') {
      buffer[i] = 0;
      return i;
    }
    buffer[i] = c;
  }
  error("String literal too long");
  return 0;
}

// 这里只做简单的判断，如果首字母是对应关键字的首字母，则直接返回关键字
static int get_keyword(char *s) {
  switch (*s) {
    case 'c':
      if (!strcmp(s, "char")) return TOKEN_CHAR;
      break;
    case 'l':
      if (!strcmp(s, "long")) return TOKEN_LONG;
    case 'i':
      if (!strcmp(s, "if")) return TOKEN_IF;
      else if (!strcmp(s, "int")) return TOKEN_INT;
      break;
    case 'e':
      if (!strcmp(s, "else")) return TOKEN_ELSE;
      else if (!strcmp(s, "enum")) return TOKEN_ENUM;
      break;
    case 'w':
      if (!strcmp(s, "while")) return TOKEN_WHILE;
      break;
    case 'f':
      if (!strcmp(s, "for")) return TOKEN_FOR;
      break;
    case 'r':
      if (!strcmp(s, "return")) return TOKEN_RETURN;
    case 'v':
      if (!strcmp(s, "void")) return TOKEN_VOID;
      break;
    case 's':
      if (!strcmp(s, "struct")) return TOKEN_STRUCT;
      break;
    case 'u':
      if (!strcmp(s, "union")) return TOKEN_UNION;
      break;
    case 't':
      if (!strcmp(s, "typedef")) return TOKEN_TYPEDEF;
      break;
  }
  return 0;
}

// 扫描 tokens
// 只有扫描到文件尾时返回 0，表示扫描结束
// 其他情况均在扫描中
int scan(struct Token *t) {
  char c;

  if (rejected_token) {
    t = rejected_token;
    rejected_token = NULL;
    return 1;
  }

  // 去掉不需要的字符
  c = skip();

  switch (c) {
    case EOF:
      t->token = TOKEN_EOF;
      return 0;
    case '+':
      if ((c = next()) == '+') {
        t->token = TOKEN_INCREASE;
      } else {
        put_back(c);
        t->token = TOKEN_PLUS;
      }
      break;
    case '-':
      if ((c = next()) == '-') {
        t->token = TOKEN_DECREASE;
      } else if (c == '>') {
        t->token = TOKEN_ARROW;
      } else {
        put_back(c);
        t->token = TOKEN_MINUS;
      }
      break;
    case '*':
      t->token = TOKEN_MULTIPLY;
      break;
    case '/':
      t->token = TOKEN_DIVIDE;
      break;
    case ';':
      t->token = TOKEN_SEMICOLON;
      break;
    case '(':
      t->token = TOKEN_LEFT_PAREN;
      break;
    case ')':
      t->token = TOKEN_RIGHT_PAREN;
      break;
    case '{':
      t->token = TOKEN_LEFT_BRACE;
      break;
    case '}':
      t->token = TOKEN_RIGHT_BRACE;
      break;
    case '[':
      t->token = TOKEN_LEFT_BRACKET;
      break;
    case ']':
      t->token = TOKEN_RIGHT_BRACKET;
      break;
    case ',':
      t->token = TOKEN_COMMA;
      break;
    case '~':
      t->token = TOKEN_INVERT;
      break;
    case '^':
      t->token = TOKEN_XOR;
      break;
    case '.':
      t->token = TOKEN_DOT;
      break;

    case '\'':
      // 如果是单引号，则扫描引号中字符的值以及尾单引号
      t->integer_value = escape_character();
      t->token = TOKEN_INTEGER_LITERAL;
      if (next() != '\'')
        error("Expected '\\'' at end of char literal");
      break;
    case '"':
      scan_string(text_buffer);
      t->token = TOKEN_STRING_LITERAL;
      break;

    case '&':
      if ((c = next()) == '&') {
        t->token = TOKEN_LOGIC_AND;
      } else {
        put_back(c);
        t->token = TOKEN_AMPERSAND;
      }
      break;
    case '|':
      if ((c = next()) == '|') {
        t->token = TOKEN_LOGIC_OR;
      } else {
        put_back(c);
        t->token = TOKEN_OR;
      }
      break;
    case '=':
      if ((c = next()) == '=') {
        t->token = TOKEN_COMPARE_EQUALS;
      } else {
        put_back(c);
        t->token = TOKEN_ASSIGN;
      }
      break;
    case '!':
      if ((c = next()) == '=') {
        t->token = TOKEN_COMPARE_NOT_EQUALS;
      } else {
        put_back(c);
        t->token = TOKEN_LOGIC_NOT;
      }
      break;
    case '<':
      if ((c = next()) == '=') {
        t->token = TOKEN_COMPARE_LESS_EQUALS;
      } else if (c == '<') {
        t->token = TOKEN_LEFT_SHIFT;
      } else {
        put_back(c);
        t->token = TOKEN_COMPARE_LESS_THAN;
      }
      break;
    case '>':
      if ((c = next()) == '=') {
        t->token = TOKEN_COMPARE_GREATER_EQUALS;
      } else if (c == '>') {
        t->token = TOKEN_RIGHT_SHIFT;
      }else {
        put_back(c);
        t->token = TOKEN_COMPARE_GREATER_THAN;
      }
      break;
    default:
      if (isdigit(c)) {
        t->token = TOKEN_INTEGER_LITERAL;
        // 如果遇到数字，则继续扫描
        t->integer_value = scan_integer(c);
        break;
      } else if (isalpha(c) || '_' == c) {
        // 如果遇到是一个字母开头的，则将其视为标识符扫描
        scan_identifier(c, text_buffer, TEXT_LENGTH);

        int token = get_keyword(text_buffer);
        if (token) {
          t->token = token;
          break;
        }
        // 如果都不是关键字，则只能说明是个标识符
        t->token = TOKEN_IDENTIFIER;
        break;
      }
      error_with_character("Unrecognised character", c);
  }

  return 1;
}
