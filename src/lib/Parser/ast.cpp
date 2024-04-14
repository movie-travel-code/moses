//===------------------------------ast.cpp--------------------------------===//
//
// This file is used to implement class stmt, decl.
// To Do: The semantic analyzer use syntax-directed pattern.
//
//===---------------------------------------------------------------------===//
#include "Parser/ast.h"
using namespace ast;

//===----------------------CompoundStmt-------------------------===//
//
void CompoundStmt::addSubStmt(StmtASTPtr stmt) {
  if (!stmt)
    SubStmts.push_back(std::move(stmt));
}

StmtASTPtr CompoundStmt::getSubStmt(std::size_t index) const {
  return SubStmts[index];
}

StmtASTPtr CompoundStmt::operator[](std::size_t index) const {
  assert(index < SubStmts.size() && "Index out of range!");
  return SubStmts[index];
}

//===---------------------------UnpackDecl-----------------------------===//
bool UnpackDecl::TypeCheckingAndTypeSetting(AnonTyPtr type) {
  std::size_t size = decls.size();
  if (type->getSubTypesNum() != size) {
    return false;
  }

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

std::vector<VarDeclPtr> UnpackDecl::operator[](std::size_t index) const {
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

void UnpackDecl::getDecls(std::vector<VarDeclPtr> &SubDecls) const {
  std::size_t size = decls.size();
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

void UnpackDecl::setCorrespondingType(std::shared_ptr<ASTType> type) {
  declType = type;
}

//===--------------------------FunctionDecl--------------------------===//
//
ReturnStmtPtr FunctionDecl::isEvalCandiateAndGetReturnStmt() const {
  if (returnType->getKind() != TypeKind::INT &&
      returnType->getKind() != TypeKind::BOOL) {
    return nullptr;
  }
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
