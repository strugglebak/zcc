
#ifndef __AST_H__
#define __AST_H__

struct ASTNode *create_ast_node(
  int operation,
  struct ASTNode *left,
  struct ASTNode *root,
  struct ASTNode *right,
  int interger_value
);

struct ASTNode *create_ast_leaf(int operation, int interger_value);

struct ASTNode *create_ast_left_node(
  int operation,
  struct ASTNode *left,
  int interger_value
);

#endif
