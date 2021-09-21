#pragma once
#include "include/Support/error.h"
#include "PreStoreToken.h"
#include "Token.h"
#include "TokenKinds.h"
#include <algorithm>
#include <cassert>
#include <cctype>
#include <fstream>
#include <iostream>
#include <string>

namespace parse {
using namespace lex;

class Scanner {
private:
  enum class State { NONE, END_OF_FILE, IDENTIFIER, NUMBER, STRING, OPERATION };

  std::string FileName;
  std::ifstream input;
  unsigned long CurLine;
  unsigned long CurCol;
  TokenLocation CurLoc;

  char CurrentChar;
  static bool errorFlag;

  State state;
  Token Tok;
  Token LastTok;

  PreStoreToken table;
  std::string buffer;

private:
  void getNextChar();
  char peekChar();
  void addToBuffer(char c);
  void reduceBuffer();
  void makeToken(tok::TokenValue tv, const TokenLocation &loc,
                 std::string name);
  void makeToken(tok::TokenValue tv, const TokenLocation &loc,
                 long intValue, std::string name);
  void makeToken(tok::TokenValue tv, const TokenLocation &loc,
                 double realValue, std::string name);
  void handleEOFState();
  void handleIdentifierState();
  void handleNumberState();
  void handleStringState();
  void handleOperationState();
  void preprocess();
  void handleLineComment();
  // void handleBlockComment();

  TokenLocation getTokenLocation() const {
    return TokenLocation(CurLine, CurCol, FileName);
  }

  void handleDigit();
  void handleXDigit();
  void handleFraction();
  void handleExponent();
  bool isOperator();

public:
  explicit Scanner(const std::string &srcFileName);
  Token getToken() const { return Tok; };
  Token getLastToken() const { return LastTok; };
  Token getNextToken();

  static bool getErrorFlag() { return errorFlag; };
  void errorReport(const std::string &msg);
  static void setErrorFlag(bool flag) { errorFlag = flag; }
};
} // namespace parse
