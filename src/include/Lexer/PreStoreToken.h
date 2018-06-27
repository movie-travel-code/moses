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
/// @brief Ԥ��Token
/// (1) �ؼ���
/// (2) �����
class PreStoreToken {
  // ��vectorֻ��Ϊ�˴洢Ԥ��Ĺؼ��ֺ������
  std::vector<Token> tokenTable;

public:
  PreStoreToken();

  // Ԥ�Ƚ��ؼ��ֲ��룬�÷���֮�󱻵���һ�Σ�Ҳ����Ԥ�Ƚ��ؼ��ֺ������װ��
  void AddToken();

  // ���Token�Ƿ��ǹؼ��֣����ڹؼ���ͨ��name�Ϳ��Ա�ʶ
  tok::TokenValue isKeyword(const std::string &lexem) const;

  // ���Token�Ƿ���������������ͨ�����־Ϳ��Ա�ʶ
  tok::TokenValue isOperator(const std::string &lexem) const;
};
} // namespace lex
} // namespace compiler

#endif