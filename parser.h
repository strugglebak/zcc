
#ifndef __PARSER_H__
#define __PARSER_H__

int convert_token_operation_2_ast_operation (int operation_in_token);
struct ASTNode *converse_token_2_ast(int previous_token_precedence);

#endif
