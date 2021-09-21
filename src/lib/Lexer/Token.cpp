//===-------------------------------Token.cpp--------------------------------===//
//
// This file is used to implement class Token.
//
//===---------------------------------------------------------------------===//
#include "Lexer/Token.h"
using namespace lex;

Token::Token(TokenValue tv, const std::string &tokenName)
    : value(tv), lexem(tokenName) {}

Token::Token()
    : value(tok::TokenValue::FILE_EOF), loc(0, 0, std::string("")), lexem(""), 
    intValue(0), realValue(0), strValue("") {}

// identifier and keyword
Token::Token(TokenValue tv, const TokenLocation &location, const std::string &name)
    : value(tv), loc(location), lexem(name), intValue(0), realValue(0), 
      strValue(name) {}

// literal
// Token::Token(TokenValue tv, const TokenLocation& location, const std::string&
// strValue, std::string name) : 	value(tok::TokenValue::FILE_EOF),
// Length(0),
// TokenLoc(location), intValue(0), realValue(0), strValue(strValue),
// name(name){}
// int
Token::Token(TokenValue tv, const TokenLocation &location, long intvalue,
             const std::string &name)
    : value(tv), loc(location), lexem(name), intValue(intvalue), realValue(0),
      strValue("") {}
// real
Token::Token(TokenValue tv, const TokenLocation &location, double realvalue,
             const std::string &name)
    : value(tv), loc(location), lexem(name), intValue(0), realValue(realvalue),
      strValue("") {}