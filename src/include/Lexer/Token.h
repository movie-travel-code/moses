#ifndef TOKEN_INCLUDE
#define TOKEN_INCLUDE
#include "TokenKinds.h"
#include <string>


namespace compiler {
namespace lex {
class TokenLocation {
  unsigned LineNumber;
  unsigned ColNumber;
  std::string FileName;

public:
  TokenLocation() : LineNumber(0), ColNumber(0), FileName("") {}

  TokenLocation(const TokenLocation &TL) {
    LineNumber = TL.LineNumber;
    ColNumber = TL.ColNumber;
    FileName = TL.FileName;
  }

  bool operator==(const TokenLocation &tokenLoc) const {
    if (LineNumber == tokenLoc.LineNumber && ColNumber == tokenLoc.ColNumber &&
        FileName == tokenLoc.FileName)
      return true;
    return false;
  }

  bool operator!=(const TokenLocation &tokenLoc) const {
    return !operator==(tokenLoc);
  }

  TokenLocation(unsigned lineNumber, unsigned colNumber, std::string fileName)
      : LineNumber(lineNumber), ColNumber(colNumber), FileName(fileName) {}

  void setTokenLineNumber(unsigned lineNumber) { LineNumber = lineNumber; }

  void setTokenColNumber(unsigned colNumber) { ColNumber = colNumber; }

  void setTokenFileName(unsigned fileName) { FileName = fileName; }

  unsigned getTokenLineNumber() const { return LineNumber; }

  unsigned getTokenColNumber() const { return ColNumber; }

  std::string getTokenFileName() const { return FileName; }

  std::string toString() {
    std::string LineStr = std::to_string(LineNumber);
    std::string ColStr = std::to_string(ColNumber);
    return FileName + ": Line: " + LineStr + ", ColumnNumber: " + ColStr;
  }
};

class Token {
  typedef tok::TokenValue TokenValue;

  TokenValue value;
  TokenLocation loc;

  std::string lexem;

  long intValue;
  double realValue;
  std::string strValue;

public:
  Token();

  Token(TokenValue tv, std::string lexem);

  Token(TokenValue tv, const TokenLocation &location, std::string lexem);

  // Token(TokenValue tv, const TokenLocation& location, const std::string&
  // strValue, std::string lexem);
  Token(TokenValue tv, const TokenLocation &location, long intvalue,
        std::string lexem);

  Token(TokenValue tv, const TokenLocation &location, double realvalue,
        std::string lexem);
  enum class TokenFlags { StatrtOfLine, LeadingSpace };

  void setKind(compiler::tok::TokenValue K) { value = K; }
  TokenValue getKind() const { return value; }

  bool is(TokenValue K) const { return K == value; }
  bool isNot(TokenValue K) const { return K != value; }

  TokenLocation &getTokenLoc() { return loc; }
  void setTokenLoc(const TokenLocation &loc) { this->loc = loc; }

  // help method
  bool isIdentifier() { return value == TokenValue::IDENTIFIER; }

  bool isAssign() {
    return (value >= TokenValue::BO_Assign) &&
           (value <= TokenValue::BO_OrAssign);
  }

  // LHS and RHS must be int type.
  bool isIntOperator() {
    if (value == TokenValue::BO_Add || value == TokenValue::BO_AddAssign ||
        value == TokenValue::BO_Sub || value == TokenValue::BO_SubAssign ||
        value == TokenValue::BO_Mul || value == TokenValue::BO_MulAssign ||
        value == TokenValue::BO_Div || value == TokenValue::BO_DivAssign)
      return true;
    return false;
  }

  // Operands must be bool type.
  bool isBoolOperator() {
    if (value == TokenValue::BO_And || value == TokenValue::BO_Or ||
        value == TokenValue::UO_Exclamatory ||
        value == TokenValue::BO_AndAssign || value == TokenValue::BO_OrAssign)
      return true;
    return false;
  }

  bool isCmpOperator() {
    if (value == TokenValue::BO_EQ || value == TokenValue::BO_GE ||
        value == TokenValue::BO_GT || value == TokenValue::BO_LE ||
        value == TokenValue::BO_LT || value == TokenValue::BO_NE)
      return true;
    return false;
  }

  bool isArithmeticOperator() {
    if (value == TokenValue::BO_Add || value == TokenValue::BO_AddAssign ||
        value == TokenValue::BO_Sub || value == TokenValue::BO_SubAssign ||
        value == TokenValue::BO_Mul || value == TokenValue::BO_MulAssign ||
        value == TokenValue::BO_Div || value == TokenValue::BO_DivAssign) {
      return true;
    }
    return false;
  }

  bool isLogicalOperator() {
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

  bool isKeyword() {
    return (value >= TokenValue::KEYWORD_var) &&
           (value <= TokenValue::KEYWORD_return);
  }

  bool isPunctuator() {
    return (value >= TokenValue::PUNCTUATOR_Left_Paren) &&
           (value <= TokenValue::PUNCTUATOR_Comma);
  }

  bool isBinaryOp() {
    return (value >= TokenValue::BO_Mul) &&
           (value <= TokenValue::BO_OrAssign) &&
           (value != TokenValue::UO_Inc || value != TokenValue::UO_Dec ||
            value != TokenValue::UO_Exclamatory);
  }

  bool isUnaryOp() {
    return (value == TokenValue::UO_Dec) || (value == TokenValue::UO_Inc) ||
           (value == TokenValue::UO_Exclamatory);
  }

  bool isCharConstant() { return value == TokenValue::CHAR_LITERAL; }
  bool isNumericConstant() {
    return value == TokenValue::REAL_LITERAL ||
           value == TokenValue::INTEGER_LITERAL;
  }
  bool isStringLiteral() { return value == TokenValue::STRING_LITERAL; }

  tok::TokenValue getValue() const { return value; }

  long getIntValue() const { return intValue; }
  double getRealValue() const { return realValue; }
  std::string getStringValue() const { return strValue; }
  std::string getLexem() { return lexem; }

  bool operator==(const Token &token) const {
    if (value == token.value && loc == token.loc) {
      return true;
    }
    return false;
  }
  bool operator!=(const Token &token) const { return !operator==(token); }
};
} // namespace lex
} // namespace compiler
#endif