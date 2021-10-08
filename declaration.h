#ifndef __DECLARATION_H__
#define __DECLARATION_H__


int parse_typedef_declaration(struct SymbolTable **composite_type);
int parse_type_of_typedef_declaration(char *name, struct SymbolTable **composite_type);
int convert_token_2_primitive_type(struct SymbolTable **composite_type);
struct SymbolTable *parse_var_declaration_statement(
  int primitive_type,
  int storage_class,
  struct SymbolTable *composite_type
);
struct ASTNode *parse_function_declaration_statement(int primitive_type);
void parse_global_declaration_statement();

#endif
