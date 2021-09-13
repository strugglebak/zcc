#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "scan.h"
#include "data.h"
#include "definations.h"
#include "helper.h"

/**
 * 检查当前 token 是否为给到的标识符，如果不是则报错
 * 如果是则扫描下一个 token
*/
void verify_token_and_fetch_next_token(int token, char *wanted_identifier) {
  if (token_from_file.token != token) {
    error_with_message("Expected", wanted_identifier);
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

void verify_while() {
  verify_token_and_fetch_next_token(TOKEN_WHILE, "while");
}

void verify_for() {
  verify_token_and_fetch_next_token(TOKEN_FOR, "for");
}

void verify_left_paren() {
  verify_token_and_fetch_next_token(TOKEN_LEFT_PAREN, "(");
}

void verify_right_paren() {
  verify_token_and_fetch_next_token(TOKEN_RIGHT_PAREN, ")");
}

void verify_left_brace() {
  verify_token_and_fetch_next_token(TOKEN_LEFT_BRACE, "{");
}

void verify_right_brace() {
  verify_token_and_fetch_next_token(TOKEN_RIGHT_BRACE, "}");
}

void verify_left_bracket() {
  verify_token_and_fetch_next_token(TOKEN_LEFT_BRACKET, "[");
}

void verify_right_bracket() {
  verify_token_and_fetch_next_token(TOKEN_RIGHT_BRACKET, "]");
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

static int generate_dump_label(void) {
  static int id = 1;
  return (id++);
}
// 递归打印 ast
void dump_ast(struct ASTNode *n, int label, int level) {
  int label_false, label_start, label_end;
  struct SymbolTable t = global_symbol_table[n->value.symbol_table_index];

  switch (n->operation) {
    case AST_IF:
      label_false = generate_dump_label();
      for (int i = 0; i < level; i++) fprintf(stdout, " ");
      fprintf(stdout, "AST_IF");
      if (n->right) {
        label_end = generate_dump_label();
        fprintf(stdout, ", end L%d", label_end);
      }
      fprintf(stdout, "\n");
      dump_ast(n->left, label_false, level+2);
      dump_ast(n->middle, NO_LABEL, level+2);
      if (n->right) dump_ast(n->right, NO_LABEL, level+2);
      return;
    case AST_WHILE:
      label_start = generate_dump_label();
      for (int i = 0; i < level; i++) fprintf(stdout, " ");
      fprintf(stdout, "AST_WHILE, start L%d\n", label_start);
      label_end = generate_dump_label();
      dump_ast(n->left, label_end, level+2);
      dump_ast(n->right, NO_LABEL, level+2);
      return;
  }

  if (n->operation == AST_GLUE) level= -2;

  if (n->left) dump_ast(n->left, NO_LABEL, level+2);
  if (n->right) dump_ast(n->right, NO_LABEL, level+2);


  for (int i = 0; i < level; i++) fprintf(stdout, " ");
  switch (n->operation) {
    case AST_GLUE:
      fprintf(stdout, "\n\n"); return;

    case AST_FUNCTION:
      fprintf(stdout, "AST_FUNCTION %s\n", t.name); return;
    case AST_PLUS:
      fprintf(stdout, "AST_PLUS\n"); return;
    case AST_MINUS:
      fprintf(stdout, "AST_MINUS\n"); return;
    case AST_MULTIPLY:
      fprintf(stdout, "AST_MULTIPLY\n"); return;
    case AST_DIVIDE:
      fprintf(stdout, "AST_DIVIDE\n"); return;
    case AST_COMPARE_EQUALS:
      fprintf(stdout, "AST_EQ\n"); return;
    case AST_COMPARE_NOT_EQUALS:
      fprintf(stdout, "AST_NE\n"); return;
    case AST_COMPARE_LESS_THAN:
      fprintf(stdout, "AST_LE\n"); return;
    case AST_COMPARE_GREATER_THAN:
      fprintf(stdout, "AST_GT\n"); return;
    case AST_COMPARE_LESS_EQUALS:
      fprintf(stdout, "AST_LE\n"); return;
    case AST_COMPARE_GREATER_EQUALS:
      fprintf(stdout, "AST_GE\n"); return;
    case AST_INTEGER_LITERAL:
      fprintf(stdout, "AST_INTLIT %d\n", n->value.interger_value); return;
    case AST_IDENTIFIER:
      n->rvalue
        ? fprintf(stdout, "AST_IDENT rval %s\n", t.name)
        : fprintf(stdout, "AST_IDENT %s\n", t.name);
      return;
    case AST_ASSIGNMENT_STATEMENT:
      fprintf(stdout, "AST_ASSIGN\n"); return;
    case AST_FUNCTION_CALL:
      fprintf(stdout, "AST_FUNCCALL %s\n", t.name); return;
    case AST_RETURN:
      fprintf(stdout, "AST_RETURN\n"); return;
    case AST_IDENTIFIER_ADDRESS:
      fprintf(stdout, "AST_ADDR %s\n", t.name); return;
    case AST_DEREFERENCE_POINTER:
      n->rvalue
        ? fprintf(stdout, "AST_DEREF rval\n")
        : fprintf(stdout, "AST_DEREF\n");
      return;
    case AST_WIDEN:
      fprintf(stdout, "AST_WIDEN\n"); return;
    case AST_SCALE:
      fprintf(stdout, "AST_SCALE %d\n", n->value.scale_size); return;
    default:
      error_with_digital("Unknown dump_ast operator", n->operation);
  }
}
