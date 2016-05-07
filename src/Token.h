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

		// help method
		bool isIdentifier() { return value == TokenValue::IDENTIFIER; }

		bool isAssign()
		{
			return (value >= TokenValue::BO_Assign) && (value <= TokenValue::BO_OrAssign);
		}

		bool isIntOperator()
		{
			if (value == TokenValue::BO_Add			|| 
				value == TokenValue::BO_AddAssign	|| 
				value == TokenValue::BO_Sub			|| 
				value == TokenValue::BO_SubAssign	|| 
				value == TokenValue::BO_Mul			|| 
				value == TokenValue::BO_MulAssign	|| 
				value == TokenValue::BO_Div			|| 
				value == TokenValue::BO_DivAssign	||
				value == TokenValue::BO_EQ			||
				value == TokenValue::BO_GE			||
				value == TokenValue::BO_GT			||
				value == TokenValue::BO_LE			||
				value == TokenValue::BO_LT			||
				value == TokenValue::BO_NE)
			{
				return true;
			}
			return false;
		}

		bool isBoolOperator()
		{
			if (value == TokenValue::BO_And			|| 
				value == TokenValue::BO_Or			|| 
				value == TokenValue::UO_Exclamatory || 
				value == TokenValue::BO_AndAssign	|| 
				value == TokenValue::BO_OrAssign	||
				value == TokenValue::BO_NE)
			{
				return true;
			}
			return false;
		}

		bool isArithmeticOperator()
		{
			if (value == TokenValue::BO_Add			||
				value == TokenValue::BO_AddAssign	||
				value == TokenValue::BO_Sub			||
				value == TokenValue::BO_SubAssign	||
				value == TokenValue::BO_Mul			||
				value == TokenValue::BO_MulAssign	||
				value == TokenValue::BO_Div			||
				value == TokenValue::BO_DivAssign )
			{
				return true;
			}
			return false;
		}

		// Note: 在moses中，int和bool类型无法转换
		bool isLogicalOperator()
		{
			if (value == TokenValue::BO_EQ			||
				value == TokenValue::BO_GE			||
				value == TokenValue::BO_GT			||
				value == TokenValue::BO_LE			||
				value == TokenValue::BO_LT			||
				value == TokenValue::BO_NE			||
				value == TokenValue::BO_And			||
				value == TokenValue::BO_Or			||
				value == TokenValue::UO_Exclamatory ||
				value == TokenValue::BO_AndAssign	||
				value == TokenValue::BO_OrAssign)
			{
				return true;
			}
			return false;
		}

		bool isKeyword() 
		{ 
			return (value >= TokenValue::KEYWORD_var) && (value <= TokenValue::KEYWORD_return); 
		}

		bool isPunctuator() 
		{ 
			return (value >= TokenValue::PUNCTUATOR_Left_Paren) && 
				(value <= TokenValue::PUNCTUATOR_Comma);
		}

		bool isBinaryOp()
		{
			return (value >= TokenValue::BO_Mul) && 
				(value <= TokenValue::BO_OrAssign) && 
				(value != TokenValue::UO_Inc || value != TokenValue::UO_Dec || value != TokenValue::UO_Exclamatory);
		}
		
		bool isUnaryOp()
		{
			return (value == TokenValue::UO_Dec) || 
				(value == TokenValue::UO_Inc) || 
				(value == TokenValue::UO_Exclamatory);
		}

		bool isCharConstant() { return value == TokenValue::CHAR_LITERAL; }
		bool isNumericConstant() 
		{ 
			return value == TokenValue::REAL_LITERAL || value == TokenValue::INTEGER_LITERAL;  
		}
		bool isStringLiteral() { return value == TokenValue::STRING_LITERAL; }

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