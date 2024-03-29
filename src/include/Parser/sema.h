//===-----------------------------------sema.h----------------------------===//
//
// This file defines the Sema class, which performs semantic analysis and
// builds ASTs. Consult Clang 2.6.
// Ad hoc syntax-directed semantic analysis.
//
// Checks of many kinds.
// (1) All identifiers are declared.
// (2) Types
// (3) Inheritance relationsships(Temporarily moses's class type doesn't have
//		inheritance).
// (4) classes defined only once.
// (5) Methods in a class defined only once(Temporarily moses's class type
//		doesn't support method.).
// (6) Reserved identifiers are mot misused.
// And others.
//
// -----------------------------------Nonsense for
// coding------------------------ Three kinds of languages: (1) statically
// typed: All or almost all checking of types is done as part of
//	   compilation(C, Java, including moses)
// (2) Dynamically typed: Almost all checking of types is done as part of
// program
//     execution(Scheme, Lisp, python, perl).
// ------------------------------------------------------------------------------
//===---------------------------------------------------------------------===//

#pragma once
#include "ASTContext.h"
#include "Lexer/scanner.h"
#include "Parser/Type.h"
#include "Support/Hasing.h"
#include "Support/SymbolTable.h"
#include <memory>
#include <string>


namespace sema {
using namespace ast;
using namespace lex;
using namespace Hashing;
using namespace Support;

/// sema - This implements sematic analysis and AST building for moses.
class Sema {
  Sema(const Sema &) = delete;
  void operator=(const Sema &) = delete;
  parse::Scanner *scan;
  ASTContext &Ctx;

private:
  std::shared_ptr<Scope> CurScope;

  /// \brief ScopeStack represents scope stack.
  std::vector<std::shared_ptr<Scope>> ScopeStack;

  /// \brief ScopeTree
  std::shared_ptr<Scope> ScopeTreeRoot;

  // class B
  // {
  //		public:
  //		class A
  //		{
  //		}
  // }
  std::vector<std::shared_ptr<ClassSymbol>> ClassStack;

  // func add() -> void
  // {
  //		func sub() -> int {}
  //		return sub;
  // }
  std::vector<std::shared_ptr<FunctionSymbol>> FunctionStack;

public:
  Sema(ASTContext &Ctx) : Ctx(Ctx) {}

public:
  // The functions for handling scope.
  // Shit Code!
  void getScannerPointer(parse::Scanner *scan) { this->scan = scan; };
  std::shared_ptr<Scope> getCurScope() { return CurScope; }
  std::shared_ptr<Scope> getTopLevelScope() { return ScopeStack[0]; }
  void PushScope(std::shared_ptr<Scope> scope);

  void PopScope();
  void PopFunctionStack();
  void PopClassStack();

  std::shared_ptr<FunctionSymbol> getFunctionStackTop() const;

  std::shared_ptr<ClassSymbol> getClassStackTop() const;

public:
  // Action routines for semantic analysis.
  // (1) Statement
  /// \brief Add a new function scope and symbol.
  void ActOnTranslationUnitStart();

  void ActOnFunctionDeclStart(const std::string &name);

  void ActOnFunctionDecl(const std::string &name,
                         std::shared_ptr<ASTType> returnType);

  StmtASTPtr ActOnFunctionDeclEnd(const std::string &name);

  void ActOnParmDecl(const std::string &name, ParmDeclPtr parm);

  StmtASTPtr ActOnConstDecl(const std::string &name,
                            std::shared_ptr<ASTType> type);

  void ActOnVarDecl(const std::string &name, VarDeclPtr VD);

  void ActOnClassDeclStart(const std::string &name);
  void ActOnCompoundStmt();

  StmtASTPtr ActOnIfStmt(SourceLocation start, SourceLocation end,
                         ExprASTPtr condtion, StmtASTPtr ThenPart,
                         StmtASTPtr ElsePart);

  std::shared_ptr<ASTType> ActOnReturnType(const std::string &name) const;

  bool ActOnBreakAndContinueStmt(bool whileContext);

  StmtASTPtr ActOnContinueStmt();

  bool ActOnReturnStmt(std::shared_ptr<ASTType> type) const;

  StmtASTPtr BuildReturnStmt();

  bool ActOnUnpackDeclElement(const std::string &name);

  UnpackDeclPtr ActOnUnpackDecl(UnpackDeclPtr unpackDecl,
                                std::shared_ptr<ASTType> type);

  bool ActOnReturnAnonymous(std::shared_ptr<ASTType> type) const;

  BinaryPtr ActOnAnonymousTypeVariableAssignment(ExprASTPtr lhs,
                                                 ExprASTPtr rhs) const;

  // (2) Expression
  ExprASTPtr ActOnConstantExpression();

  std::shared_ptr<ASTType>
  ActOnCallExpr(const std::string &calleeName,
                std::vector<std::shared_ptr<ASTType>> Args,
                FunctionDeclPtr &FD);

  ExprASTPtr ActOnUnaryOperator();

  ExprASTPtr ActOnPostfixUnaryOperator();

  ExprASTPtr ActOnBinaryOperator(ExprASTPtr lhs, Token tok, ExprASTPtr rhs);

  VarDeclPtr ActOnDeclRefExpr(const std::string &name);

  ExprASTPtr ActOnStringLiteral();

  ExprASTPtr ActOnMemberAccessExpr(ExprASTPtr lhs, Token tok);

  ExprASTPtr ActOnExprStmt();

  bool ActOnConditionExpr(std::shared_ptr<ASTType> type) const;

  std::shared_ptr<ASTType> ActOnParmDeclUserDefinedType(Token tok) const;

  std::shared_ptr<ASTType> ActOnVarDeclUserDefinedType(Token tok) const;

  // (3) help method
  bool isInFunctionContext() const { return FunctionStack.size() != 0; }

  ExprASTPtr ActOnDecOrIncExpr(ExprASTPtr rhs);

  ExprASTPtr ActOnUnarySubExpr(ExprASTPtr rhs);

  ExprASTPtr ActOnUnaryExclamatoryExpr(ExprASTPtr rhs);

  // To Do: implement RAII
  class CompoundScopeRAII {};
  // To Do: implement RAII
  class FunctionScopeRAII {};

  std::shared_ptr<Scope> getScopeStackBottom() const;

private:
  std::shared_ptr<Scope> getScopeStackTop() const;

  void errorReport(const std::string &msg) const;

  UnpackDeclPtr unpackDeclTypeChecking(UnpackDeclPtr unpackDecl,
                                       std::shared_ptr<ASTType> initType) const;
};
} // namespace sema
