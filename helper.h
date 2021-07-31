#ifndef __HELPER_H__
#define __HELPER_H__

void verify_token_and_fetch_next_token(int token, char *wanted_identifier);
void verify_semicolon();
void verify_identifier();
void verify_if();

void error(char *string);
void error_with_message(char *string, char *message);
void error_with_digital(char *string, int digital);
void error_with_character(char *string, char character);

#endif
