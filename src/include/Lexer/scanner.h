#ifndef Scanner_INCLUDE
#define Scanner_INCLUDE
#include "../Support/error.h"
#include "PreStoreToken.h"
#include "Token.h"
#include "TokenKinds.h"
#include <algorithm>
#include <cassert>
#include <cctype>
#include <fstream>
#include <iostream>
#include <string>


namespace compiler {
namespace parse {
using namespace compiler::lex;

// Scanner - ����ԭʼ�ı��õ�һ��token��
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

  // Ԥ��Token
  PreStoreToken table;
  // ���ڽ�����Token
  std::string buffer;

private:
  /// @brief ��ȡ��һ���ַ�
  void getNextChar();

  /// @brief ��ǰ��һ���ַ������ڴʷ�����
  char peekChar();

  /// @brief ����ǰ�ַ����뻺��
  void addToBuffer(char c);
  /// @brief �����ǰ��ʧ�ܣ������
  void reduceBuffer();

  // �����ַ���Token
  void makeToken(compiler::tok::TokenValue tv, const TokenLocation &loc,
                 std::string name);
  // ��������ֵToken������ֵ
  void makeToken(compiler::tok::TokenValue tv, const TokenLocation &loc,
                 long intValue, std::string name);
  // ����double��Token
  void makeToken(compiler::tok::TokenValue tv, const TokenLocation &loc,
                 double realValue, std::string name);

  // �����ļ���β
  void handleEOFState();
  // �����ʶ��
  void handleIdentifierState();
  // ��������
  void handleNumberState();
  // �����ַ���
  void handleStringState();
  // ���������ʱ��״̬
  void handleOperationState();

  // ����հ׼�ע��
  void preprocess();
  // ������ע��
  void handleLineComment();
  // �����ע��
  // void handleBlockComment();

  // ������ǰλ�õ�TokenLocation
  TokenLocation getTokenLocation() const {
    return TokenLocation(CurLine, CurCol, FileName);
  }

  // �ʷ�������������
  void handleDigit();
  void handleXDigit();

  // �ʷ�����С������
  void handleFraction();
  // �ʷ�����ָ������
  void handleExponent();

  // �ж��Ƿ��������
  bool isOperator();

public:
  // ��ʾ���캯��
  // ��ʵ�ʷ��������������Լ������ɣ���������ʽ����ת���Ŀ���
  explicit Scanner(const std::string &srcFileName);
  // ��ȡ��ǰToken
  Token getToken() const { return Tok; };
  Token getLastToken() const { return LastTok; };
  // ��ȡ��һ��Token����������ᱻParser����
  Token getNextToken();

  // ���ش�������
  static bool getErrorFlag() { return errorFlag; };

  // ����
  void errorReport(const std::string &msg);

  static void setErrorFlag(bool flag) { errorFlag = flag; }
};
} // namespace parse
} // namespace compiler

#endif