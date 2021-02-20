
#ifndef __PARSER_H__
#define __PARSER_H__

int convert_token_operation_2_ast_operation (int operation_in_token);
struct ASTNode *converse_token_2_ast();
struct ASTNode *converse_token_2_ast_v2(int previous_token_precedence);
struct ASTNode *converse_token_2_multiplicative_ast();
struct ASTNode *converse_token_2_additive_ast();

#endif
