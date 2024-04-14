#pragma once
#include "TokenKinds.h"
#include <string>


namespace lex {
class TokenLocation {
  std::size_t LineNumber;
  std::size_t ColNumber;
  std::string FileName;

public:
  TokenLocation() : LineNumber(0), ColNumber(0), FileName("") {}

  TokenLocation(const TokenLocation &TL) {
    LineNumber = TL.LineNumber;
    ColNumber = TL.ColNumber;
    FileName = TL.FileName;
  }

  TokenLocation& operator=(const TokenLocation &TL) {
    LineNumber = TL.LineNumber;
    ColNumber = TL.ColNumber;
    FileName = TL.FileName;
    return *this;
  }

  bool operator==(const TokenLocation &tokenLoc) const {
    if (LineNumber == tokenLoc.LineNumber && ColNumber == tokenLoc.ColNumber &&
        FileName == tokenLoc.FileName)
      return true;
    return false;
  }

  [[nodiscard]] bool operator!=(const TokenLocation &tokenLoc) const {
    return !operator==(tokenLoc);
  }

  TokenLocation(std::size_t lineNumber, std::size_t colNumber, std::string fileName)
      : LineNumber(lineNumber), ColNumber(colNumber), FileName(fileName) {}

  void setTokenLineNumber(std::size_t lineNumber) { LineNumber = lineNumber; }
  void setTokenColNumber(std::size_t colNumber) { ColNumber = colNumber; }
  void setTokenFileName(std::size_t fileName) { FileName = fileName; }
  [[nodiscard]] unsigned long getTokenLineNumber() const { return LineNumber; }
  [[nodiscard]] unsigned long getTokenColNumber() const { return ColNumber; }
  [[nodiscard]] const std::string& getTokenFileName() const { return FileName; }
  std::string toString() {
    std::string LineStr = std::to_string(LineNumber);
    std::string ColStr = std::to_string(ColNumber);
    return FileName + ": Line: " + LineStr + ", ColumnNumber: " + ColStr;
  }
};

class Token {
  using TokenValue = tok::TokenValue;

  TokenValue value;
  TokenLocation loc;

  std::string lexem;

  long intValue;
  double realValue;
  std::string strValue;

public:
  Token();

  Token(TokenValue tv, const std::string &lexem);

  Token(TokenValue tv, const TokenLocation &location, const std::string &lexem);

  // Token(TokenValue tv, const TokenLocation& location, const std::string&
  // strValue, std::string lexem);
  Token(TokenValue tv, const TokenLocation &location, long intvalue,
        const std::string &lexem);

  Token(TokenValue tv, const TokenLocation &location, double realvalue,
        const std::string &lexem);
  enum class TokenFlags { StatrtOfLine, LeadingSpace };

  void setKind(tok::TokenValue K) { value = K; }
  [[nodiscard]] TokenValue getKind() const { return value; }

  [[nodiscard]] bool is(TokenValue K) const { return K == value; }
  [[nodiscard]] bool isNot(TokenValue K) const { return K != value; }

  [[nodiscard]] TokenLocation &getTokenLoc() { return loc; }
  void setTokenLoc(const TokenLocation &loc) { this->loc = loc; }

  // help method
  [[nodiscard]] bool isIdentifier() { return value == TokenValue::IDENTIFIER; }

  [[nodiscard]] bool isAssign() {
    return (value >= TokenValue::BO_Assign) &&
           (value <= TokenValue::BO_OrAssign);
  }

  // LHS and RHS must be int type.
  [[nodiscard]] bool isIntOperator() {
    if (value == TokenValue::BO_Add || value == TokenValue::BO_AddAssign ||
        value == TokenValue::BO_Sub || value == TokenValue::BO_SubAssign ||
        value == TokenValue::BO_Mul || value == TokenValue::BO_MulAssign ||
        value == TokenValue::BO_Div || value == TokenValue::BO_DivAssign)
      return true;
    return false;
  }

  // Operands must be bool type.
  [[nodiscard]] bool isBoolOperator() {
    if (value == TokenValue::BO_And || value == TokenValue::BO_Or ||
        value == TokenValue::UO_Exclamatory ||
        value == TokenValue::BO_AndAssign || value == TokenValue::BO_OrAssign)
      return true;
    return false;
  }

  [[nodiscard]] bool isCmpOperator() {
    if (value == TokenValue::BO_EQ || value == TokenValue::BO_GE ||
        value == TokenValue::BO_GT || value == TokenValue::BO_LE ||
        value == TokenValue::BO_LT || value == TokenValue::BO_NE)
      return true;
    return false;
  }

  [[nodiscard]] bool isArithmeticOperator() {
    if (value == TokenValue::BO_Add || value == TokenValue::BO_AddAssign ||
        value == TokenValue::BO_Sub || value == TokenValue::BO_SubAssign ||
        value == TokenValue::BO_Mul || value == TokenValue::BO_MulAssign ||
        value == TokenValue::BO_Div || value == TokenValue::BO_DivAssign) {
      return true;
    }
    return false;
  }

  [[nodiscard]] bool isLogicalOperator() {
    if (value == TokenValue::BO_EQ || value == TokenValue::BO_GE ||
        value == TokenValue::BO_GT || value == TokenValue::BO_LE ||
        value == TokenValue::BO_LT || value == TokenValue::BO_NE ||
        value == TokenValue::BO_And || value == TokenValue::BO_Or ||
        value == TokenValue::UO_Exclamatory ||
        value == TokenValue::BO_AndAssign || value == TokenValue::BO_OrAssign) {
      return true;
    }
    return false;
  }

  [[nodiscard]] bool isKeyword() {
    return (value >= TokenValue::KEYWORD_var) &&
           (value <= TokenValue::KEYWORD_return);
  }

  [[nodiscard]] bool isPunctuator() {
    return (value >= TokenValue::PUNCTUATOR_Left_Paren) &&
           (value <= TokenValue::PUNCTUATOR_Comma);
  }

  [[nodiscard]] bool isBinaryOp() {
    return (value >= TokenValue::BO_Mul) &&
           (value <= TokenValue::BO_OrAssign) &&
           value != TokenValue::UO_Inc && value != TokenValue::UO_Dec &&
            value != TokenValue::UO_Exclamatory;
  }

  [[nodiscard]] bool isUnaryOp() {
    return (value == TokenValue::UO_Dec) || (value == TokenValue::UO_Inc) ||
           (value == TokenValue::UO_Exclamatory);
  }

  [[nodiscard]] bool isCharConstant() { return value == TokenValue::CHAR_LITERAL; }
  [[nodiscard]] bool isNumericConstant() {
    return value == TokenValue::REAL_LITERAL ||
           value == TokenValue::INTEGER_LITERAL;
  }
  [[nodiscard]] bool isStringLiteral() { return value == TokenValue::STRING_LITERAL; }

  [[nodiscard]] tok::TokenValue getValue() const { return value; }

  [[nodiscard]] long getIntValue() const { return intValue; }
  [[nodiscard]] double getRealValue() const { return realValue; }
  [[nodiscard]] const std::string& getStringValue() const { return strValue; }
  [[nodiscard]] std::string getLexem() { return lexem; }

  [[nodiscard]] bool operator==(const Token &token) const {
    if (value == token.value && loc == token.loc) {
      return true;
    }
    return false;
  }
  [[nodiscard]] bool operator!=(const Token &token) const { return !operator==(token); }
};
} // namespace lex
