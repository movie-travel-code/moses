//===---------------------------PreStoreToken.cpp-------------------------===//
//
// IdentifierTable Implemention
//
//===---------------------------------------------------------------------===//
#include "Lexer/PreStoreToken.h"
using namespace lex;
using namespace tok;

#define INSERT(KEYWORD, STR)                                                   \
  Token II_##KEYWORD(TokenValue::##KEYWORD, STR);                    \
  tokenTable.push_back(II_##KEYWORD)

TokenValue PreStoreToken::isKeyword(const std::string &lexem) const {
  TokenValue tokValue = TokenValue::UNKNOWN;
  for_each(tokenTable.begin(), tokenTable.end(),
           [&tokValue, lexem](Token token) {
             if (token.getLexem() == lexem &&
                 token.getKind() >= TokenValue::KEYWORD_var)
               tokValue = token.getKind();
           });
  return tokValue;
}

TokenValue PreStoreToken::isOperator(const std::string &lexem) const {
  TokenValue tokValue = TokenValue::UNKNOWN;
  for_each(tokenTable.begin(), tokenTable.end(),
           [&tokValue, lexem](Token token) {
             if (token.getLexem() == lexem &&
                 token.getKind() < TokenValue::KEYWORD_if)
               tokValue = token.getKind();
           });
  return tokValue;
}