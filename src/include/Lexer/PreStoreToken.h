//===---------------------------PreStoreToken.h--------------------------===//
//
// This file define class PreStoreToken, and this class used to save
// keyword.
//
//===--------------------------------------------------------------------===//
#pragma once
#include "Token.h"
#include <algorithm>
#include <cassert>
#include <string>
#include <utility>
#include <vector>

using namespace tok;

namespace lex {
class PreStoreToken {
  std::vector<Token> tokenTable{
      {TokenValue::KEYWORD_var, "var"},
      {TokenValue::KEYWORD_const, "const"},
      {TokenValue::KEYWORD_if, "if"},
      {TokenValue::KEYWORD_else, "else"},
      {TokenValue::KEYWORD_break, "break"},
      {TokenValue::KEYWORD_while, "while"},
      {TokenValue::KEYWORD_continue, "continue"},
      {TokenValue::KEYWORD_int, "int"},
      {TokenValue::KEYWORD_bool, "bool"},
      {TokenValue::KEYWORD_class, "class"},
      {TokenValue::KEYWORD_return, "return"},
      {TokenValue::KEYWORD_func, "func"},
      {TokenValue::KEYWORD_void, "void"},
      {TokenValue::BOOL_TRUE, "true"},
      {TokenValue::BOOL_FALSE, "false"},
      {TokenValue::BO_Mul, "*"},
      {TokenValue::BO_Div, "/"},
      {TokenValue::BO_Rem, "%"},
      {TokenValue::BO_Add, "+"},
      {TokenValue::BO_Sub, "-"},
      {TokenValue::BO_LT, "<"},
      {TokenValue::BO_GT, ">"},
      {TokenValue::BO_LE, "<="},
      {TokenValue::BO_GE, ">="},
      {TokenValue::BO_EQ, "=="},
      {TokenValue::BO_NE, "!="},
      {TokenValue::BO_And, "&&"},
      {TokenValue::BO_Or, "||"},
      {TokenValue::BO_AndAssign, "&&=="},
      {TokenValue::BO_OrAssign, "||="},
      {TokenValue::BO_Assign, "="},
      {TokenValue::BO_MulAssign, "*="},
      {TokenValue::BO_DivAssign, "/="},
      {TokenValue::BO_RemAssign, "%="},
      {TokenValue::BO_AddAssign, "+="},
      {TokenValue::BO_SubAssign, "-="},
      {TokenValue::UO_Dec, "--"},
      {TokenValue::UO_Inc, "++"},
      {TokenValue::UO_Exclamatory, "!"},
      {TokenValue::PUNCTUATOR_Left_Paren, "("},
      {TokenValue::PUNCTUATOR_Right_Paren, ")"},
      {TokenValue::PUNCTUATOR_Left_Brace, "{"},
      {TokenValue::PUNCTUATOR_Right_Brace, "}"},
      {TokenValue::PUNCTUATOR_Arrow, "->"},
      {TokenValue::PUNCTUATOR_Colon, ":"},
      {TokenValue::PUNCTUATOR_Semicolon, ";"},
      {TokenValue::PUNCTUATOR_Member_Access, "."},
      {TokenValue::PUNCTUATOR_Comma, ","}
  };

public:
  [[nodiscard]] tok::TokenValue isKeyword(const std::string &lexem) const;

  [[nodiscard]] tok::TokenValue isOperator(const std::string &lexem) const;
};
} // namespace lex
