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
		// 执行默认的拷贝行为即可
		// 拷贝string数据成员时，会执行string的复制运算符
		TokenLocation() : LineNumber(0), ColNumber(0), FileName(""){}

		TokenLocation(const TokenLocation& TL)
		{
			LineNumber = TL.LineNumber;
			ColNumber = TL.ColNumber;
			FileName = TL.FileName;
		}

		// 默认inline
		bool operator==(const TokenLocation& tokenLoc) const
		{
			if (LineNumber == tokenLoc.LineNumber && ColNumber == tokenLoc.ColNumber && FileName == tokenLoc.FileName)
				return true;
			return false;
		}

		// 这里编译器会将其inline掉的，不用考虑函数调用的开销
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

	/// @brief Token用来表示一个经过词法分析的一个词法单元
	/// 由于一些Token方法都是比较短的，所以设置inline函数
	class Token
	{
		typedef tok::TokenValue TokenValue;

		TokenValue value;
		TokenLocation loc;
		
		std::string lexem;

		// 关于一些token的常量值
		long  intValue;
		double realValue;
		std::string strValue;
	public:
		Token();

		// 拷贝构造函数
		// 编译器会合成默认的拷贝构造函数
		// Token(const Token& token);
		// 下面是一系列的构造函数

		// 两个参数的Token只用于关键字和预定义的一些操作符等
		Token(TokenValue tv, std::string lexem);

		// 标识符
		Token(TokenValue tv, const TokenLocation& location, std::string lexem);

		// 字符串
		// Token(TokenValue tv, const TokenLocation& location, const std::string& strValue, std::string lexem);
		// 整型
		Token(TokenValue tv, const TokenLocation& location, long intvalue, std::string lexem);
		// 小数
		Token(TokenValue tv, const TokenLocation& location, double realvalue, std::string lexem);
		enum class TokenFlags
		{
			StatrtOfLine,
			LeadingSpace
		};

		// help函数用来设置Token的value
		void setKind(compiler::tok::TokenValue K) { value = K; }
		TokenValue getKind() const { return value; }

		bool is(TokenValue K) const { return K == value; }
		bool isNot(TokenValue K) const { return K != value; }

		TokenLocation& getTokenLoc() { return loc; }
		void setTokenLoc(const TokenLocation& loc) { this->loc = loc; }

		// 一些辅助判断函数
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

		// 获取Token的Token值
		tok::TokenValue getValue() const { return value; }


		// 获取Token的常量值
		long getIntValue() const { return intValue; }
		double getRealValue() const { return realValue; }
		std::string getStringValue() const { return strValue; }
		std::string getLexem() { return lexem; }

		// 两个token是否相等
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