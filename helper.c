#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include "scan.h"
#include "data.h"
#include "definations.h"
#include "helper.h"

static int dump_label_id = 1;
static int generate_dump_label(void) {
  return (dump_label_id++);
}

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
void verify_colon() {
  verify_token_and_fetch_next_token(TOKEN_COLON, ":");
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

void verify_return() {
  verify_token_and_fetch_next_token(TOKEN_RETURN, "return");
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

void verify_comma() {
  verify_token_and_fetch_next_token(TOKEN_COMMA, ",");
}



void error(char *string) {
  fprintf(stderr, "%s on line %d of %s\n", string, line, global_input_filename);
  fclose(output_file);
  unlink(global_output_filename);
  exit(1);
}

void error_with_message(char *string, char *message) {
  fprintf(stderr, "%s:%s on line %d of %s\n", string, message, line, global_input_filename);
  fclose(output_file);
  unlink(global_output_filename);
  exit(1);
}

void error_with_digital(char *string, int digital) {
  fprintf(stderr, "%s:%d on line %d of %s\n", string, digital, line, global_input_filename);
  fclose(output_file);
  unlink(global_output_filename);
  exit(1);
}

void error_with_character(char *string, char character) {
  fprintf(stderr, "%s:%c on line %d of %s\n", string, character, line, global_input_filename);
  fclose(output_file);
  unlink(global_output_filename);
  exit(1);
}

// 递归打印 ast
void dump_ast(struct ASTNode *n, int label, int level) {
  int label_false, label_start, label_end;
  struct SymbolTable *t = n->symbol_table;
  int i;

  switch (n->operation) {
    case AST_IF:
      label_false = generate_dump_label();
      for (i = 0; i < level; i++) fprintf(stdout, " ");
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
      for (i = 0; i < level; i++) fprintf(stdout, " ");
      fprintf(stdout, "AST_WHILE, start L%d\n", label_start);
      label_end = generate_dump_label();
      dump_ast(n->left, label_end, level+2);
      dump_ast(n->right, NO_LABEL, level+2);
      return;
  }

  if (n->operation == AST_GLUE) level= -2;

  if (n->left) dump_ast(n->left, NO_LABEL, level+2);
  if (n->right) dump_ast(n->right, NO_LABEL, level+2);


  for (i = 0; i < level; i++) fprintf(stdout, " ");
  switch (n->operation) {
    case AST_GLUE:
      fprintf(stdout, "\n\n"); return;

    case AST_FUNCTION:
      fprintf(stdout, "AST_FUNCTION %s\n", t->name); return;
    case AST_FUNCTION_CALL:
      fprintf(stdout, "AST_FUNCCALL %s\n", t->name); return;
    case AST_RETURN:
      fprintf(stdout, "AST_RETURN\n"); return;
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
      fprintf(stdout, "AST_INTLIT %d\n", n->ast_node_integer_value); return;
    case AST_STRING_LITERAL:
      fprintf(stdout, "AST_STRLIT rval label L%d\n", n->ast_node_integer_value); return;
    case AST_IDENTIFIER:
      n->rvalue
        ? fprintf(stdout, "AST_IDENT rval %s\n", t->name)
        : fprintf(stdout, "AST_IDENT %s\n", t->name);
      return;
    case AST_ASSIGN:
      fprintf(stdout, "AST_ASSIGN\n"); return;
    case AST_IDENTIFIER_ADDRESS:
      fprintf(stdout, "AST_ADDR %s\n", t->name); return;
    case AST_DEREFERENCE_POINTER:
      n->rvalue
        ? fprintf(stdout, "AST_DEREF rval\n")
        : fprintf(stdout, "AST_DEREF\n");
      return;
    case AST_WIDEN:
      fprintf(stdout, "AST_WIDEN\n"); return;
    case AST_SCALE:
      fprintf(stdout, "AST_SCALE %d\n", n->ast_node_scale_size); return;

    case AST_PRE_INCREASE:
      fprintf(stdout, "AST_PREINC %s\n", n->symbol_table->name); return;
    case AST_PRE_DECREASE:
      fprintf(stdout, "AST_PREDEC %s\n", n->symbol_table->name); return;
    case AST_POST_INCREASE:
      fprintf(stdout, "AST_POSTINC\n"); return;
    case AST_POST_DECREASE:
      fprintf(stdout, "AST_POSTDEC\n"); return;
    case AST_NEGATE:
      fprintf(stdout, "AST_NEGATE\n"); return;
    case AST_BREAK:
      fprintf(stdout, "AST_BREAK\n"); return;
    case AST_CONTINUE:
      fprintf(stdout, "AST_CONTINUE\n"); return;
    case AST_CASE:
      fprintf(stdout, "AST_CASE %d\n", n->ast_node_integer_value); return;
    case AST_DEFAULT:
      fprintf(stdout, "AST_DEFAULT\n"); return;
    case AST_SWITCH:
      fprintf(stdout, "AST_SWITCH\n"); return;
    case AST_TYPE_CASTING:
      fprintf(stdout, "AST_CAST %d\n", n->primitive_type); return;
    case AST_ASSIGN_PLUS:
      fprintf(stdout, "AST_ASPLUS\n"); return;
    case AST_ASSIGN_MINUS:
      fprintf(stdout, "AST_ASMINUS\n"); return;
    case AST_ASSIGN_MULTIPLY:
      fprintf(stdout, "AST_ASMULTIPLY\n"); return;
    case AST_ASSIGN_DIVIDE:
      fprintf(stdout, "AST_ASDIVIDE\n"); return;
    default:
      error_with_digital("Unknown dump_ast operator", n->operation);
  }
}

static void dump_single_symbol(struct SymbolTable *t, int indent) {
  int i;

  for (i = 0; i < indent; i++) printf(" ");
  switch (t->primitive_type & (~0xf)) {
    case PRIMITIVE_VOID: printf("void "); break;
    case PRIMITIVE_CHAR: printf("char "); break;
    case PRIMITIVE_INT: printf("int "); break;
    case PRIMITIVE_LONG: printf("long "); break;
    case PRIMITIVE_STRUCT: printf("struct %s ", t->composite_type ? t->composite_type->name : t->name); break;
    case PRIMITIVE_UNION: printf("union %s ", t->composite_type ? t->composite_type->name : t->name); break;
    default: printf("unknown primitive type ");
  }

  for (i = 0; i < (t->primitive_type & 0xf); i++) printf("*");
  printf("%s", t->name);

  switch (t->structural_type) {
    case STRUCTURAL_VARIABLE: break;
    case STRUCTURAL_FUNCTION: printf("()"); break;
    case STRUCTURAL_ARRAY: printf("[]"); break;
    default: printf(" unknown structural type");
  }

  switch (t->storage_class) {
    case STORAGE_CLASS_GLOBAL: printf(": global"); break;
    case STORAGE_CLASS_LOCAL: printf(": local"); break;
    case STORAGE_CLASS_FUNCTION_PARAMETER: printf(": param"); break;
    case STORAGE_CLASS_EXTERN: printf(": extern"); break;
    case STORAGE_CLASS_STATIC: printf(": static"); break;
    case STORAGE_CLASS_STRUCT: printf(": struct"); break;
    case STORAGE_CLASS_UNION: printf(": union"); break;
    case STORAGE_CLASS_MEMBER: printf(": member"); break;
    case STORAGE_CLASS_ENUM_TYPE: printf(": enumtype"); break;
    case STORAGE_CLASS_ENUM_VALUE: printf(": enumval"); break;
    case STORAGE_CLASS_TYPEDEF: printf(": typedef"); break;
    default: printf(": unknown storage class");
  }

  switch (t->structural_type) {
    case STRUCTURAL_VARIABLE:
      if (t->storage_class == STORAGE_CLASS_ENUM_VALUE)
        printf(", value %d\n", t->symbol_table_position);
      else
        printf(", size %d\n", t->size);
      break;
    case STRUCTURAL_FUNCTION: printf(", %d params\n", t->element_number); break;
    case STRUCTURAL_ARRAY: printf(", %d elems, size %d\n", t->element_number, t->size); break;
  }

  switch (t->primitive_type & (~0xf)) {
    case PRIMITIVE_STRUCT:
    case PRIMITIVE_UNION: dump_single_symbol_table(t->member, NULL, 4);
  }

  switch (t->structural_type) {
  case STRUCTURAL_FUNCTION:
    dump_single_symbol_table(t->member, NULL, 4);
  }
}

void dump_single_symbol_table(
  struct SymbolTable *head,
  char *name,
  int indent
) {
  struct SymbolTable *t;

  if (head && name) printf("%s\n--------\n", name);
  for (t = head; t; t = t->next) dump_single_symbol(t, indent);
}

void dump_symbol_table() {
  dump_single_symbol_table(global_head, "Global", 0);
  printf("\n");
  dump_single_symbol_table(enum_head, "Enums", 0);
  printf("\n");
  dump_single_symbol_table(typedef_head, "Typedefs", 0);
}
