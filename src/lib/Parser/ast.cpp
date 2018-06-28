//===------------------------------ast.cpp--------------------------------===//
//
// This file is used to implement class stmt, decl.
// To Do: The semantic analyzer use syntax-directed pattern.
//
//===---------------------------------------------------------------------===//
#include "../../include/Parser/ast.h"
using namespace compiler::ast;

//===----------------------CompoundStmt-------------------------===//
//
void CompoundStmt::addSubStmt(StmtASTPtr stmt) {
  if (!stmt)
    SubStmts.push_back(std::move(stmt));
}

StmtASTPtr CompoundStmt::getSubStmt(unsigned index) const {
  return SubStmts[index];
}

StmtASTPtr CompoundStmt::operator[](unsigned index) const {
  assert(index < SubStmts.size() && "Index out of range!");
  return SubStmts[index];
}

//===---------------------------UnpackDecl-----------------------------===//
//
/// \brief ��Ҫ�������͵ļ�������
/// ���磺 var {num, {lhs, rhs}} �� type
/// To Do: ������������⣬ָ�뱩¶
bool UnpackDecl::TypeCheckingAndTypeSetting(AnonTyPtr type) {
  unsigned size = decls.size();
  // (1) ���unpack decl��size��Anonymous type����type�Ƿ���ͬ
  if (type->getSubTypesNum() != size) {
    return false;
  }

  // (2) �ݹ�������ÿ��element.
  // Note: ��������������ݽ�������type�����Բ�����type��������ָ��
  // ʹ��ͬһ��ԭ��ָ���ʼ����������ָ�룬�п��ܻ��������ָ�롣
  for (unsigned index = 0; index < size; index++) {
    if (UnpackDeclPtr unpackd =
            std::dynamic_pointer_cast<UnpackDecl>(decls[index])) {
      if (AnonTyPtr anonyt = std::dynamic_pointer_cast<AnonymousType>(
              type->getSubType(index))) {
        return unpackd->TypeCheckingAndTypeSetting(anonyt);
      } else {
        return false;
      }
    } else {
      if (AnonTyPtr anonyt = std::dynamic_pointer_cast<AnonymousType>(
              type->getSubType(index))) {
        return false;
      }
    }
  }
  return true;
}

/// \brief ��ȡdecl name
std::vector<VarDeclPtr> UnpackDecl::operator[](unsigned index) const {
  std::vector<VarDeclPtr> SubDecls;
  if (UnpackDeclPtr unpackd =
          std::dynamic_pointer_cast<UnpackDecl>(decls[index])) {
    unpackd->getDecls(SubDecls);
  }

  if (VarDeclPtr var = std::dynamic_pointer_cast<VarDecl>(decls[index])) {
    SubDecls.push_back(var);
  }
  return SubDecls;
}

/// \brief ��ȡdecl names
void UnpackDecl::getDecls(std::vector<VarDeclPtr> &SubDecls) const {
  unsigned size = decls.size();
  for (unsigned index = 0; index < size; index++) {
    if (UnpackDeclPtr unpackd =
            std::dynamic_pointer_cast<UnpackDecl>(decls[index])) {
      unpackd->getDecls(SubDecls);
    }

    if (VarDeclPtr var = std::dynamic_pointer_cast<VarDecl>(decls[index])) {
      SubDecls.push_back(var);
    }
  }
}

void UnpackDecl::setCorrespondingType(std::shared_ptr<Type> type) {
  declType = type;
}

//===--------------------------FunctionDecl--------------------------===//
//
ReturnStmtPtr FunctionDecl::isEvalCandiateAndGetReturnStmt() const {
  if (returnType->getKind() != TypeKind::INT &&
      returnType->getKind() != TypeKind::BOOL) {
    return nullptr;
  }
  // (1) �������Ƿ�ֻ��һ�� return stmt
  if (CompoundStmtPtr body =
          std::dynamic_pointer_cast<CompoundStmt>(funcBody)) {
    if (body->getSize() != 1)
      return nullptr;
    if (ReturnStmtPtr returnStmt =
            std::dynamic_pointer_cast<ReturnStatement>(body->getSubStmt(0))) {
      return returnStmt;
    }
    return nullptr;
  }
  return nullptr;
}

bool FunctionDecl::endsWithReturn() const {
  if (CompoundStmtPtr body =
          std::dynamic_pointer_cast<CompoundStmt>(funcBody)) {
    // ends with return stmt.
    if (ReturnStmtPtr ret = std::dynamic_pointer_cast<ReturnStatement>(
            (*body)[body->getSize() - 1])) {
      return true;
    }
  }
  return false;
}