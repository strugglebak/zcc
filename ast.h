
#ifndef __AST_H__
#define __AST_H__

struct ASTNode *create_ast_node(
  int operation,
  int primitive_type,
  struct ASTNode *left,
  struct ASTNode *middle,
  struct ASTNode *right,
  int integer_value,
  struct SymbolTable *symbol_table
);
struct ASTNode *create_ast_leaf(
  int operation,
  int primitive_type,
  int integer_value,
  struct SymbolTable *symbol_table
);
struct ASTNode *create_ast_left_node(
  int operation,
  int primitive_type,
  struct ASTNode *left,
  int integer_value,
  struct SymbolTable *symbol_table
);

#endif
