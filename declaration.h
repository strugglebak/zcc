#ifndef __DECLARATION_H__
#define __DECLARATION_H__


int convert_token_2_primitive_type(
  struct SymbolTable **composite_type,
  int *storage_class
);
int convert_multiply_token_2_primitive_type(int primitive_type);
int convert_literal_token_2_integer_value(int primitive_type);
int convert_type_casting_token_2_primitive_type(struct SymbolTable **composite_type);

int parse_typedef_declaration(struct SymbolTable **composite_type);
int parse_type_of_typedef_declaration(char *name, struct SymbolTable **composite_type);
struct SymbolTable *parse_function_declaration(
  int primitive_type,
  char *function_name,
  struct SymbolTable *composite_type,
  int storage_class
);
int parse_declaration_list(
  struct SymbolTable **composite_type,
  int storage_class,
  int end_token,
  int end_token_2,
  struct ASTNode **glue_tree
);
void parse_global_declaration();

#endif
