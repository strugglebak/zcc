#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "scan.h"
#include "data.h"
#include "definations.h"

/**
 * 检查当前 token 是否为给到的标识符，如果不是则报错
 * 如果是则扫描下一个 token
*/
void verify_token_and_fetch_next_token(int token, char *wanted_identifier) {
  if (token_from_file.token != token) {
    printf("%s expected on line %d\n", wanted_identifier, line);
    exit(1);
  }
  scan(&token_from_file);
}

void verify_semicolon() {
  verify_token_and_fetch_next_token(TOKEN_SEMICOLON, ";");
}

void verify_identifier() {
  verify_token_and_fetch_next_token(TOKEN_IDENTIFIER, "identifier");
}

void verify_if() {
  verify_token_and_fetch_next_token(TOKEN_IF, "if");
}

void error(char *string) {
  fprintf(stderr, "%s on line %d\n", string, line);
  exit(1);
}

void error_with_message(char *string, char *message) {
  fprintf(stderr, "%s:%s on line %d\n", string, message, line);
  exit(1);
}

void error_with_digital(char *string, int digital) {
  fprintf(stderr, "%s:%d on line %d\n", string, digital, line);
  exit(1);
}

void error_with_character(char *string, char character) {
  fprintf(stderr, "%s:%c on line %d\n", string, character, line);
  exit(1);
}
