//===---------------------------PreStoreToken.h--------------------------===//
//
// This file define class PreStoreToken, and this class used to save
// keyword.
//
//===--------------------------------------------------------------------===//
#ifndef PRESTORETOKEN_INCLUDE
#define PRESTORETOKEN_INCLUDE
#include "Token.h"
#include <algorithm>
#include <cassert>
#include <string>
#include <utility>
#include <vector>


namespace compiler {
namespace lex {
class PreStoreToken {
  std::vector<Token> tokenTable;

public:
  PreStoreToken();

  void AddToken();

  tok::TokenValue isKeyword(const std::string &lexem) const;

  tok::TokenValue isOperator(const std::string &lexem) const;
};
} // namespace lex
} // namespace compiler

#endif