//===---------------------------PreStoreToken.cpp-------------------------===//
//
// IdentifierTable Implemention
//
//===---------------------------------------------------------------------===//
#include "include/Lexer/PreStoreToken.h"
using namespace compiler::lex;
using namespace compiler::tok;

PreStoreToken::PreStoreToken() { AddToken(); }

#define INSERT(KEYWORD, STR)                                                   \
  Token II_##KEYWORD(compiler::TokenValue::##KEYWORD, STR);                    \
  tokenTable.push_back(II_##KEYWORD)

// Ϊ��ʶ���
void PreStoreToken::AddToken() {
  using namespace compiler::tok;
  tokenTable.push_back(Token(TokenValue::KEYWORD_var, "var"));
  tokenTable.push_back(Token(TokenValue::KEYWORD_const, "const"));
  tokenTable.push_back(Token(TokenValue::KEYWORD_if, "if"));
  tokenTable.push_back(Token(TokenValue::KEYWORD_else, "else"));
  tokenTable.push_back(Token(TokenValue::KEYWORD_break, "break"));
  tokenTable.push_back(Token(TokenValue::KEYWORD_while, "while"));
  tokenTable.push_back(Token(TokenValue::KEYWORD_continue, "continue"));
  tokenTable.push_back(Token(TokenValue::KEYWORD_int, "int"));
  tokenTable.push_back(Token(TokenValue::KEYWORD_bool, "bool"));
  tokenTable.push_back(Token(TokenValue::KEYWORD_class, "class"));
  tokenTable.push_back(Token(TokenValue::KEYWORD_return, "return"));
  tokenTable.push_back(Token(TokenValue::KEYWORD_func, "func"));
  tokenTable.push_back(Token(TokenValue::KEYWORD_void, "void"));

  tokenTable.push_back(Token(TokenValue::BOOL_TRUE, "true"));
  tokenTable.push_back(Token(TokenValue::BOOL_FALSE, "false"));

  tokenTable.push_back(Token(TokenValue::BO_Mul, "*"));
  tokenTable.push_back(Token(TokenValue::BO_Div, "/"));
  tokenTable.push_back(Token(TokenValue::BO_Rem, "%"));
  tokenTable.push_back(Token(TokenValue::BO_Add, "+"));
  tokenTable.push_back(Token(TokenValue::BO_Sub, "-"));
  tokenTable.push_back(Token(TokenValue::BO_LT, "<"));
  tokenTable.push_back(Token(TokenValue::BO_GT, ">"));
  tokenTable.push_back(Token(TokenValue::BO_LE, "<="));
  tokenTable.push_back(Token(TokenValue::BO_GE, ">="));
  tokenTable.push_back(Token(TokenValue::BO_EQ, "=="));
  tokenTable.push_back(Token(TokenValue::BO_NE, "!="));
  tokenTable.push_back(Token(TokenValue::BO_And, "&&"));
  tokenTable.push_back(Token(TokenValue::BO_Or, "||"));
  tokenTable.push_back(Token(TokenValue::BO_AndAssign, "&&="));
  tokenTable.push_back(Token(TokenValue::BO_OrAssign, "||="));

  tokenTable.push_back(Token(TokenValue::BO_Assign, "="));
  tokenTable.push_back(Token(TokenValue::BO_MulAssign, "*="));
  tokenTable.push_back(Token(TokenValue::BO_DivAssign, "/="));
  tokenTable.push_back(Token(TokenValue::BO_RemAssign, "%="));
  tokenTable.push_back(Token(TokenValue::BO_AddAssign, "+="));
  tokenTable.push_back(Token(TokenValue::BO_SubAssign, "-="));

  tokenTable.push_back(Token(TokenValue::UO_Dec, "--"));
  tokenTable.push_back(Token(TokenValue::UO_Inc, "++"));
  tokenTable.push_back(Token(TokenValue::UO_Exclamatory, "!"));

  tokenTable.push_back(Token(TokenValue::PUNCTUATOR_Left_Paren, "("));
  tokenTable.push_back(Token(TokenValue::PUNCTUATOR_Right_Paren, ")"));

  // tokenTable.push_back(Token(TokenValue::PUNCTUATOR_Left_Square, "["));
  // tokenTable.push_back(Token(TokenValue::PUNCTUATOR_Right_Square, "]"));
  tokenTable.push_back(Token(TokenValue::PUNCTUATOR_Left_Brace, "{"));
  tokenTable.push_back(Token(TokenValue::PUNCTUATOR_Right_Brace, "}"));
  tokenTable.push_back(Token(TokenValue::PUNCTUATOR_Arrow, "->"));
  tokenTable.push_back(Token(TokenValue::PUNCTUATOR_Colon, ":"));
  tokenTable.push_back(Token(TokenValue::PUNCTUATOR_Semicolon, ";"));
  tokenTable.push_back(Token(TokenValue::PUNCTUATOR_Member_Access, "."));
  tokenTable.push_back(Token(TokenValue::PUNCTUATOR_Comma, ","));
}

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