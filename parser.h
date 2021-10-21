
#ifndef __PARSER_H__
#define __PARSER_H__

struct ASTNode *parse_expression_list(int end_token);
struct ASTNode *converse_token_2_ast(int previous_token_precedence);
struct ASTNode *convert_function_call_2_ast();
struct ASTNode *convert_prefix_expression_2_ast();
struct ASTNode *convert_postfix_expression_2_ast();
struct ASTNode *convert_array_access_2_ast();
struct ASTNode *convert_member_access_2_ast(int with_pointer);

#endif
