//===---------------------------PreStoreToken.cpp-------------------------===//
// 
// IdentifierTable Implemention
// 
//===---------------------------------------------------------------------===//
#include "../../include/Lexer/PreStoreToken.h"

using namespace compiler;
PreStoreToken::PreStoreToken()
{
	AddToken();
}

#define INSERT(KEYWORD, STR)	\
	Token II_##KEYWORD(compiler::tok::TokenValue::##KEYWORD, STR); \
	tokenTable.push_back(II_##KEYWORD)

// 为了识别出
void PreStoreToken::AddToken()
{
	using namespace compiler::tok;
	tokenTable.push_back(Token(TokenValue::KEYWORD_var, "var"));
	tokenTable.push_back(Token(TokenValue::KEYWORD_const, "const"));
	tokenTable.push_back(Token(TokenValue::KEYWORD_if, "if"));
	tokenTable.push_back(Token(TokenValue::KEYWORD_else, "else"));
	tokenTable.push_back(Token(TokenValue::KEYWORD_break, "break"));
	tokenTable.push_back(Token(TokenValue::KEYWORD_while, "while"));
	tokenTable.push_back(Token(TokenValue::KEYWORD_continue, "continue"));
	tokenTable.push_back(Token(TokenValue::KEYWORD_int, "int"));
	tokenTable.push_back(Token(TokenValue::KEYWORD_bool, "bool"));
	tokenTable.push_back(Token(TokenValue::KEYWORD_class, "class"));
	tokenTable.push_back(Token(TokenValue::KEYWORD_return, "return"));
	tokenTable.push_back(Token(TokenValue::KEYWORD_func, "func"));
	tokenTable.push_back(Token(TokenValue::KEYWORD_void, "void"));

	tokenTable.push_back(Token(TokenValue::BOOL_TRUE, "true"));
	tokenTable.push_back(Token(TokenValue::BOOL_FALSE, "false"));	

	tokenTable.push_back(Token(TokenValue::BO_Mul, "*"));
	tokenTable.push_back(Token(TokenValue::BO_Div, "/"));
	tokenTable.push_back(Token(TokenValue::BO_Rem, "%"));
	tokenTable.push_back(Token(TokenValue::BO_Add, "+"));
	tokenTable.push_back(Token(TokenValue::BO_Sub, "-"));
	tokenTable.push_back(Token(TokenValue::BO_LT, "<"));
	tokenTable.push_back(Token(TokenValue::BO_GT, ">"));
	tokenTable.push_back(Token(TokenValue::BO_LE, "<="));
	tokenTable.push_back(Token(TokenValue::BO_GE, ">="));
	tokenTable.push_back(Token(TokenValue::BO_EQ, "=="));
	tokenTable.push_back(Token(TokenValue::BO_NE, "!="));
	tokenTable.push_back(Token(TokenValue::BO_And, "&&"));
	tokenTable.push_back(Token(TokenValue::BO_Or, "||"));
	tokenTable.push_back(Token(TokenValue::BO_AndAssign, "&&="));
	tokenTable.push_back(Token(TokenValue::BO_OrAssign, "||="));

	tokenTable.push_back(Token(TokenValue::BO_Assign, "="));
	tokenTable.push_back(Token(TokenValue::BO_MulAssign, "*="));
	tokenTable.push_back(Token(TokenValue::BO_DivAssign, "/="));
	tokenTable.push_back(Token(TokenValue::BO_RemAssign, "%="));
	tokenTable.push_back(Token(TokenValue::BO_AddAssign, "+="));
	tokenTable.push_back(Token(TokenValue::BO_SubAssign, "-="));



	// To Do: 修改文法，添加自增和自减
	tokenTable.push_back(Token(TokenValue::UO_Dec, "--"));
	tokenTable.push_back(Token(TokenValue::UO_Inc, "++"));
	tokenTable.push_back(Token(TokenValue::UO_Exclamatory, "!"));

	tokenTable.push_back(Token(TokenValue::PUNCTUATOR_Left_Paren, "("));
	tokenTable.push_back(Token(TokenValue::PUNCTUATOR_Right_Paren, ")"));

	// To Do: 修改文法，添加左右括号。由于现在没有设计数组类型。
	// tokenTable.push_back(Token(TokenValue::PUNCTUATOR_Left_Square, "["));
	// tokenTable.push_back(Token(TokenValue::PUNCTUATOR_Right_Square, "]"));
	tokenTable.push_back(Token(TokenValue::PUNCTUATOR_Left_Brace, "{"));
	tokenTable.push_back(Token(TokenValue::PUNCTUATOR_Right_Brace, "}"));
	tokenTable.push_back(Token(TokenValue::PUNCTUATOR_Arrow, "->"));
	tokenTable.push_back(Token(TokenValue::PUNCTUATOR_Colon, ":"));
	tokenTable.push_back(Token(TokenValue::PUNCTUATOR_Semicolon, ";"));
	tokenTable.push_back(Token(TokenValue::PUNCTUATOR_Member_Access, "."));
	tokenTable.push_back(Token(TokenValue::PUNCTUATOR_Comma, ","));
}

// 查找符号表
/*
tok::TokenValue PreStoreToken::Lookup(const Token& token)
{
for (auto item : tokenTable)
{
if (*item == token)
return token.getValue();
}
return tok::TokenValue::IDENTIFIER;
}
*/

tok::TokenValue PreStoreToken::isKeyword(const std::string& lexem) const
{
	tok::TokenValue tokValue = tok::TokenValue::UNKNOWN;
	// 使用for_each算法，遍历token列表，如果找到同名并且TokenKind为关键字的value则返回true
	for_each(tokenTable.begin(), tokenTable.end(), [&tokValue, lexem](Token token)
	{ 
		if (token.getLexem() == lexem &&  token.getKind() >= tok::TokenValue::KEYWORD_var)
			tokValue = token.getKind();
	}
	);
	return tokValue;
}

tok::TokenValue PreStoreToken::isOperator(const std::string& lexem) const
{
	tok::TokenValue tokValue = tok::TokenValue::UNKNOWN;
	for_each(tokenTable.begin(), tokenTable.end(), [&tokValue, lexem](Token token)
	{
		if (token.getLexem() == lexem && token.getKind() < tok::TokenValue::KEYWORD_if)
			tokValue = token.getKind();
	}
	);
	return tokValue;
}