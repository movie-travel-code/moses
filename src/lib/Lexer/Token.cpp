//===-------------------------------Token.cpp--------------------------------===//
//
// This file is used to implement class Token.
//
//===---------------------------------------------------------------------===//
#include "../../include/Lexer/Token.h"
using namespace compiler::lex;

Token::Token(TokenValue tv, std::string tokenName)
    : value(tv), lexem(tokenName) {}

Token::Token()
    : value(tok::TokenValue::FILE_EOF), loc(0, 0, std::string("")), intValue(0),
      realValue(0), lexem(""), strValue("") {}

// identifier and keyword
Token::Token(TokenValue tv, const TokenLocation &location, std::string name)
    : value(tv), loc(location), intValue(0), realValue(0), lexem(name),
      strValue(name) {}

// literal
// Token::Token(TokenValue tv, const TokenLocation& location, const std::string&
// strValue, std::string name) : 	value(tok::TokenValue::FILE_EOF),
// Length(0),
// TokenLoc(location), intValue(0), realValue(0), strValue(strValue),
// name(name){}
// int
Token::Token(TokenValue tv, const TokenLocation &location, long intvalue,
             std::string name)
    : value(tv), loc(location), intValue(intvalue), realValue(0), lexem(name),
      strValue("") {}
// real
Token::Token(TokenValue tv, const TokenLocation &location, double realvalue,
             std::string name)
    : value(tv), loc(location), intValue(0), realValue(realvalue), lexem(name),
      strValue("") {}