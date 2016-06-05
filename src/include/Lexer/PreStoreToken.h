//===---------------------------PreStoreToken.h--------------------------===//
//
// This file define class PreStoreToken, and this class used to save 
// keyword.
//
//===--------------------------------------------------------------------===//
#ifndef PRESTORETOKEN_INCLUDE
#define PRESTORETOKEN_INCLUDE
#include <string>
#include <utility>
#include <vector>
#include <cassert>
#include <algorithm>
#include "Token.h"

namespace compiler
{
	namespace lex
	{
		/// @brief 预设Token
		/// (1) 关键字
		/// (2) 运算符
		class PreStoreToken
		{
			// 该vector只是为了存储预设的关键字和运算符
			std::vector<Token> tokenTable;
		public:
			PreStoreToken();

			// 预先将关键字插入，该方法之后被调用一次，也就是预先将关键字和运算符装入
			void AddToken();

			// 检查Token是否是关键字，由于关键字通过name就可以标识
			tok::TokenValue isKeyword(const std::string& lexem) const;

			// 检查Token是否是运算符，运算符通过名字就可以标识
			tok::TokenValue isOperator(const std::string& lexem) const;
		};
	}
}

#endif