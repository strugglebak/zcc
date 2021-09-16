
#ifndef __PARSER_H__
#define __PARSER_H__

int convert_token_operation_2_ast_operation (int operation_in_token);
struct ASTNode *converse_token_2_ast(int previous_token_precedence);
struct ASTNode *convert_function_call_2_ast();
struct ASTNode *convert_prefix_expression_2_ast();
struct ASTNode *convert_postfix_expression_2_ast();
struct ASTNode *convert_array_access_2_ast();

#endif
