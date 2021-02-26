#include <stdio.h>
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
