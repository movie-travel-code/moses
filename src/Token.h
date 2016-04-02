#ifndef TOKEN_INCLUDE
#define TOKEN_INCLUDE
#include <string>
#include "TokenKinds.h"

namespace compiler
{
	class TokenLocation
	{
		unsigned LineNumber;
		unsigned ColNumber;
		std::string FileName;

	public:
		// ִ��Ĭ�ϵĿ�����Ϊ����
		// ����string���ݳ�Աʱ����ִ��string�ĸ��������
		TokenLocation() : LineNumber(0), ColNumber(0), FileName(""){}

		TokenLocation(const TokenLocation& TL)
		{
			LineNumber = TL.LineNumber;
			ColNumber = TL.ColNumber;
			FileName = TL.FileName;
		}

		// Ĭ��inline
		bool operator==(const TokenLocation& tokenLoc) const
		{
			if (LineNumber == tokenLoc.LineNumber && ColNumber == tokenLoc.ColNumber && FileName == tokenLoc.FileName)
				return true;
			return false;
		}

		// ����������Ὣ��inline���ģ����ÿ��Ǻ������õĿ���
		bool operator!=(const TokenLocation& tokenLoc) const
		{
			return !operator==(tokenLoc);
		}

		TokenLocation(unsigned lineNumber, unsigned colNumber, std::string fileName) :
			LineNumber(lineNumber), ColNumber(colNumber), FileName(fileName) {}

		void setTokenLineNumber(unsigned lineNumber) { LineNumber = lineNumber; }

		void setTokenColNumber(unsigned colNumber) { ColNumber = colNumber; }

		void setTokenFileName(unsigned fileName) { FileName = fileName; }

		unsigned getTokenLineNumber() const { return LineNumber; }

		unsigned getTokenColNumber() const { return ColNumber; }

		std::string getTokenFileName() const{ return FileName; }

		std::string toString()
		{
			std::string LineStr = std::to_string(LineNumber);
			std::string ColStr = std::to_string(ColNumber);
			return FileName + ": Line: " + LineStr + ", ColumnNumber: " + ColStr;
		}
	};

	/// @brief Token������ʾһ�������ʷ�������һ���ʷ���Ԫ
	/// ����һЩToken�������ǱȽ϶̵ģ���������inline����
	class Token
	{
		typedef tok::TokenValue TokenValue;

		TokenValue value;
		TokenLocation loc;
		
		std::string lexem;

		// ����һЩtoken�ĳ���ֵ
		long  intValue;
		double realValue;
		std::string strValue;
	public:
		Token();

		// �������캯��
		// ��������ϳ�Ĭ�ϵĿ������캯��
		// Token(const Token& token);
		// ������һϵ�еĹ��캯��

		// ����������Tokenֻ���ڹؼ��ֺ�Ԥ�����һЩ��������
		Token(TokenValue tv, std::string lexem);

		// ��ʶ��
		Token(TokenValue tv, const TokenLocation& location, std::string lexem);

		// �ַ���
		// Token(TokenValue tv, const TokenLocation& location, const std::string& strValue, std::string lexem);
		// ����
		Token(TokenValue tv, const TokenLocation& location, long intvalue, std::string lexem);
		// С��
		Token(TokenValue tv, const TokenLocation& location, double realvalue, std::string lexem);
		enum class TokenFlags
		{
			StatrtOfLine,
			LeadingSpace
		};

		// help������������Token��value
		void setKind(compiler::tok::TokenValue K) { value = K; }
		TokenValue getKind() const { return value; }

		bool is(TokenValue K) const { return K == value; }
		bool isNot(TokenValue K) const { return K != value; }

		TokenLocation& getTokenLoc() { return loc; }
		void setTokenLoc(const TokenLocation& loc) { this->loc = loc; }

		// һЩ�����жϺ���
		bool isIdentifier(TokenValue Tok) { return Tok == TokenValue::IDENTIFIER; }
		bool isKeyword(TokenValue Tok) { return Tok >= TokenValue::KEYWORD_var && Tok <= TokenValue::KEYWORD_return; }
		bool isPunctuator(TokenValue Tok) 
		{ 
			return Tok >= TokenValue::PUNCTUATOR_Left_Paren && 
				Tok <= TokenValue::PUNCTUATOR_Comma;
		}
		bool isCharConstant(TokenValue Tok) { return Tok == TokenValue::CHAR_LITERAL; }
		bool isNumericConstant(TokenValue Tok) { return Tok == TokenValue::REAL_LITERAL || Tok == TokenValue::INTEGER_LITERAL;  }
		bool isStringLiteral(TokenValue Tok) { return Tok == TokenValue::STRING_LITERAL; }

		// ��ȡToken��Tokenֵ
		tok::TokenValue getValue() const { return value; }


		// ��ȡToken�ĳ���ֵ
		long getIntValue() const { return intValue; }
		double getRealValue() const { return realValue; }
		std::string getStringValue() const { return strValue; }
		std::string getLexem() { return lexem; }

		// ����token�Ƿ����
		bool operator==(const Token& token) const
		{
			if (value == token.value && loc == token.loc)
			{
				return true;
			}
			return false;
		}
		bool operator!=(const Token& token) const
		{
			return !operator==(token);
		}
	};

}
#endif