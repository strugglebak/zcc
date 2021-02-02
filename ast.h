struct ASTNode *create_ast_node(
  int operation,
  struct ASTNode *left,
  struct ASTNode *right,
  int interger_value
);

struct ASTNode *create_ast_leaf(int operation, int interger_value);

struct ASTNode *create_ast_left_node(
  int operation,
  struct ASTNode *left,
  int interger_value
);