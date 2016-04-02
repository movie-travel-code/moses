#ifndef Scanner_INCLUDE
#define Scanner_INCLUDE
#include <fstream>
#include <iostream>
#include <string>
#include <cctype>
#include <algorithm>
#include <cassert>
#include "TokenKinds.h"
#include "error.h"
#include "PreStoreToken.h"
#include "Token.h"

namespace compiler
{
	namespace parse
	{
		using namespace compiler;

		// Scanner - 解析原始文本得到一个token流
		class Scanner
		{
		private:
			enum class State
			{
				NONE,
				END_OF_FILE,
				IDENTIFIER,
				NUMBER,
				STRING,
				OPERATION
			};

			std::string FileName;
			std::ifstream input;
			unsigned long CurLine;
			unsigned long CurCol;
			TokenLocation CurLoc;

			char CurrentChar;
			static bool errorFlag;

			State state;
			Token Tok;

			// 预设Token
			PreStoreToken table;
			// 正在解析的Token
			std::string buffer;

		private:
			/// @brief 获取下一个字符
			void getNextChar();

			/// @brief 向前看一个字符，用于词法分析
			char peekChar();

			/// @brief 将当前字符加入缓冲
			void addToBuffer(char c);
			/// @brief 如果向前看失败，则回退
			void reduceBuffer();

			// 创建字符串Token
			void makeToken(compiler::tok::TokenValue tv, const TokenLocation& loc, std::string name);
			// 创建整数值Token，字面值
			void makeToken(compiler::tok::TokenValue tv, const TokenLocation& loc, long intValue, std::string name);
			// 创建double型Token
			void makeToken(compiler::tok::TokenValue tv, const TokenLocation& loc, double realValue, std::string name);

			// 处理文件结尾
			void handleEOFState();
			// 处理标识符
			void handleIdentifierState();
			// 处理整数
			void handleNumberState();
			// 处理字符串
			void handleStringState();
			// 处理运算符时的状态
			void handleOperationState();

			// 处理空白及注释
			void preprocess();
			// 处理单行注释
			void handleLineComment();
			// 处理块注释
			// void handleBlockComment();

			// 创建当前位置的TokenLocation
			TokenLocation getTokenLocation() const
			{
				return TokenLocation(CurLine, CurCol, FileName);
			}

			// 词法分析数字类型
			void handleDigit();
			void handleXDigit();

			// 词法分析小数类型
			void handleFraction();
			// 词法分析指数类型
			void handleExponent();

			// 判断是否是运算符
			bool isOperator();

		public:
			// 显示构造函数
			// 其实词法分析器对象都是自己的生成，不存在隐式类型转换的可能
			explicit Scanner(const std::string& srcFileName);
			// 获取当前Token
			Token getToken() const { return Tok; };
			// 获取下一个Token，这个方法会被Parser调用
			Token getNextToken();

			// 返回错误类型
			static bool getErrorFlag() { return errorFlag; };

			// 报错
			void errorReport(const std::string& msg);

			static void setErrorFlag(bool flag) { errorFlag = flag; }
		};
	}
}

#endif