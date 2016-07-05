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
			IDENTIFIER,					// ��ʶ��
			INTEGER_LITERAL,			// �������ֳ���
			REAL_LITERAL,				// ʵ������
			CHAR_LITERAL,				// �ַ�����
			STRING_LITERAL,				// �ַ�������ֵ

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
			// To Do: �����������
			// PUNCTUATOR_Left_Square,		// [
			// PUNCTUATOR_Right_Square,	// ]
			PUNCTUATOR_Left_Brace,		// {
			PUNCTUATOR_Right_Brace,		// }
			PUNCTUATOR_Arrow,			// ->

			PUNCTUATOR_Colon,		// :
			PUNCTUATOR_Semicolon,	// ;
			PUNCTUATOR_Member_Access,	// .
			PUNCTUATOR_Comma,	// ,

			KEYWORD_var,				// ��������
			KEYWORD_const,				// const��������
			KEYWORD_class,				// class����ؼ���
			KEYWORD_if,					// ������if
			KEYWORD_else,				// ������else
			KEYWORD_break,		
			KEYWORD_while,				// ������while
			// To Do: ���for���������
			KEYWORD_continue,
			KEYWORD_return,				// return stmt
			KEYWORD_int,				// int
			KEYWORD_bool,				// bool
			KEYWORD_func,				// ��������
			KEYWORD_void,				// ��������void
			BOOL_TRUE,					// Bool true ���泣��
			BOOL_FALSE,					// Bool false ���泣��

			FILE_EOF,
			UNKNOWN
		};
	}
}
#endif