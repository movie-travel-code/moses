//===----------------------------TokenKinds.h-----------------------------===//
//
// This file defines class TokenValue. TokenValue is the key class for 
// lexer and parser.
//
//===---------------------------------------------------------------------===//
#ifndef TOKENKINDS_INCLUDE
#define TOKENKINDS_INCLUDE
namespace compiler
{
	namespace tok
	{
		enum class TokenValue : unsigned short
		{
			IDENTIFIER,					// 标识符
			INTEGER_LITERAL,			// 整型数字常量
			REAL_LITERAL,				// 实数常量
			CHAR_LITERAL,				// 字符常量
			STRING_LITERAL,				// 字符串字面值

			BO_Mul, BO_Div, BO_Rem,		// * / %
			BO_Add, BO_Sub,				// + -
			BO_LT, BO_GT, BO_LE, BO_GE,	// < > <= >=
			BO_EQ, BO_NE,				// == !=
			UO_Inc, UO_Dec,				// ++ --
			UO_Exclamatory,				// !

			BO_Assign, BO_MulAssign,	// = *=
			BO_DivAssign, BO_RemAssign,	// /= %=
			BO_AddAssign, BO_SubAssign,	// += -=
			
			BO_AndAssign,				// &&=
			BO_OrAssign,				// ||=
			BO_And,						// &&
			BO_Or,						// ||

			PUNCTUATOR_Left_Paren,		// (
			PUNCTUATOR_Right_Paren,		// )
			// To Do: 设计数组类型
			// PUNCTUATOR_Left_Square,		// [
			// PUNCTUATOR_Right_Square,	// ]
			PUNCTUATOR_Left_Brace,		// {
			PUNCTUATOR_Right_Brace,		// }
			PUNCTUATOR_Arrow,			// ->

			PUNCTUATOR_Colon,		// :
			PUNCTUATOR_Semicolon,	// ;
			PUNCTUATOR_Member_Access,	// .
			PUNCTUATOR_Comma,	// ,

			KEYWORD_var,				// 变量定义
			KEYWORD_const,				// const变量定义
			KEYWORD_class,				// class定义关键字
			KEYWORD_if,					// 控制流if
			KEYWORD_else,				// 控制流else
			KEYWORD_break,		
			KEYWORD_while,				// 控制流while
			// To Do: 添加for控制流语句
			KEYWORD_continue,
			KEYWORD_return,				// return stmt
			KEYWORD_int,				// int
			KEYWORD_bool,				// bool
			KEYWORD_func,				// 函数定义
			KEYWORD_void,				// 函数返回void
			BOOL_TRUE,					// Bool true 字面常量
			BOOL_FALSE,					// Bool false 字面常量

			FILE_EOF,
			UNKNOWN
		};
	}
}
#endif