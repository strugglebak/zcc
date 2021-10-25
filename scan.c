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
  char c, l;

  // 由于上一次读取到了不是数字的字符，所以这里相当于一个 buffer，直接返回上一次读取后的值即可
  if (putback_buffer) {
    c = putback_buffer;
    putback_buffer = 0;
    return c;
  }

  // 返回了上一次读取后的值之后，再次从文件刚刚的位置的下一位读取
  c = fgetc(input_file);
  // 处理预编译处理过后的头文件，这些头文件会加载到 main.c 中
  // 这些头文件一般长这样
  // # 1 "z.c"
  // # 1 "<built-in>"
  // # 1 "<command-line>"
  // # 1 "z.c"
  // # 1 "include/stdio.h" 1
  // # 1 "include/stddef.h" 1
  // typedef long size_t;
  // # 5 "include/stdio.h" 2
  // typedef char * FILE;
  // FILE *fopen(char *pathname, char *mode);
  // ...
  // # 2 "z.c" 2
  // int main() {...}

  // 先从 '#' 开始
  while (start_line && c == '#') {
    start_line = 0;
    // 跳过 '#'
    scan(&token_from_file);
    // 如果不是 '1' 报错
    if (token_from_file.token != TOKEN_INTEGER_LITERAL)
      error_with_message("Expecting pre-processor line number, got", text_buffer);
    // 把 '1' 赋值给 l
    l = token_from_file.integer_value;

    // 跳过 '1'，此时来到的是文件路径/文件名 "z.c"
    scan(&token_from_file);
    if (token_from_file.token != TOKEN_STRING_LITERAL)
      error_with_message("Expecting pre-processor file name, got", text_buffer);

    // 以 '<>' 包起来的不用检查
    if (text_buffer[0] != '<') {
      if (strcmp(text_buffer, global_input_filename))
        global_input_filename = strdup(text_buffer);
      line = l;
    }

    // 跳过行尾继续下一个
    while ((c = fgetc(input_file) != '\n'));
    c = fgetc(input_file);
    start_line = 1;
  }

  start_line = 0;
  if ('\n' == c) {
    line ++;
    start_line = 1;
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
    '\r' == c ||
    '\f' == c
  ) {
    // 遇到以上的字符就继续读取
    c = next();
  }

  return (c);
}

static int get_the_position_of_the_charater(char *s, int c) {
  // s = "0123456789"
  // s 这里是字符串的首地址
  int i;
  for (i = 0; s[i] != '\0'; i++)
    if (s[i] == (char) c)
      return i;
  return -1;
}

// 从输入的 file 中扫描并返回一个 integer 字符
static int scan_integer(char c) {
  // 默认 10 进制
  int k, value = 0, radix = 10;

  if (c == '0') {
    if ((c = next()) == 'x') {
      // 16 进制
      radix = 16;
      c = next();
    } else
      // 8 进制
      radix = 8;
  }

  while (((k = get_the_position_of_the_charater("0123456789abcdef", tolower(c))) >= 0)) {
    if (k >= radix)
      error_with_character("invalid digit in integer literal", c);
    value = value * radix + k;
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

// 解析类似于 \0x41 这样的字符
static int hex_character() {
  int c, h, n = 0, flag = 0;

  while (isxdigit(c = next())) {
    // char 变成 int
    h = get_the_position_of_the_charater("0123456789abcdef", tolower(c));
    n = n * 16 + h;
    flag = 1;
  }

  put_back(c);

  if (!flag) error("missing digits after '\\x'");
  if (n > 255) error("value out of range after '\\x'");

  return n;
}

// 扫描单引号中的字符
// 转义部分字符，这里暂时不考虑 8 进制或者其他类型的字符
static int escape_character() {
  int c = next(), i, c2;
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
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
        for (i = c2 = 0; isdigit(c) && c < '8'; c = next()) {
          if (++i > 3) break;
          c2 = c2 * 8 + (c - '0');
        }
        put_back(c);
        return c2;
      case 'x':
        return hex_character();
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
      else if (!strcmp(s, "continue")) return TOKEN_CONTINUE;
      else if (!strcmp(s, "case")) return TOKEN_CASE;
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
      else if (!strcmp(s, "extern")) return TOKEN_EXTERN;
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
      else if (!strcmp(s, "switch")) return TOKEN_SWITCH;
      else if (!strcmp(s, "sizeof")) return TOKEN_SIZEOF;
      else if (!strcmp(s, "static")) return TOKEN_STATIC;
      break;
    case 'u':
      if (!strcmp(s, "union")) return TOKEN_UNION;
      break;
    case 't':
      if (!strcmp(s, "typedef")) return TOKEN_TYPEDEF;
      break;
    case 'b':
      if (!strcmp(s, "break")) return TOKEN_BREAK;
      break;
    case 'd':
      if (!strcmp(s, "default")) return TOKEN_DEFAULT;
      break;
  }
  return 0;
}

// 扫描 tokens
// 只有扫描到文件尾时返回 0，表示扫描结束
// 其他情况均在扫描中
int scan(struct Token *t) {
  char c, token_type;

  // 如果提前找到了 token，就直接返回这个 token
  if (look_ahead_token.token) {
    t->token = look_ahead_token.token;
    t->token_string = look_ahead_token.token_string;
    t->integer_value = look_ahead_token.integer_value;
    look_ahead_token.token = 0;
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
      } else if (c == '=') {
        t->token = TOKEN_ASSIGN_PLUS;
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
      } else if (c == '=') {
        t->token = TOKEN_ASSIGN_MINUS;
      } else if (isdigit(c)) {
        // 有可能是个负数
        t->integer_value = -scan_integer(c);
        t->token = TOKEN_INTEGER_LITERAL;
      } else {
        put_back(c);
        t->token = TOKEN_MINUS;
      }
      break;
    case '*':
      if ((c = next()) == '=') {
        t->token = TOKEN_ASSIGN_MULTIPLY;
      } else {
        put_back(c);
        t->token = TOKEN_MULTIPLY;
      }
      break;
    case '/':
      if ((c = next()) == '=') {
        t->token = TOKEN_ASSGIN_DIVIDE;
      } else {
        put_back(c);
        t->token = TOKEN_DIVIDE;
      }
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
    case ':':
      t->token = TOKEN_COLON;
      break;
    case '?':
      t->token = TOKEN_QUESTION;
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

  t->token_string = token_string[t->token];
  // printf("scan '%s' -> (%s)\n", t->token_string, text_buffer);
  return 1;
}
