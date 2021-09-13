#ifndef __HELPER_H__
#define __HELPER_H__

#include "definations.h"

void verify_token_and_fetch_next_token(int token, char *wanted_identifier);
void verify_semicolon();
void verify_identifier();
void verify_if();
void verify_while();
void verify_for();
void verify_left_paren();
void verify_right_paren();
void verify_left_brace();
void verify_right_brace();
void verify_left_bracket();
void verify_right_bracket();

void error(char *string);
void error_with_message(char *string, char *message);
void error_with_digital(char *string, int digital);
void error_with_character(char *string, char character);

void dump_ast(struct ASTNode *n, int label, int level);

#endif
