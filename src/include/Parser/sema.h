//===-----------------------------------sema.h----------------------------===//
//
// This file defines the Sema class, which performs semantic analysis and
// builds ASTs. Consult Clang 2.6.
// Ad hoc syntax-directed semantic analysis.
//
// Checks of many kinds.
// (1) All identifiers are declared.
// (2) Types (�����������֮���﷨���ϵ�������ʽ�������Ӧ���ͣ� ��fully typed)
// (3) Inheritance relationsships(Temporarily moses's class type doesn't have
//		inheritance).
// (4) classes defined only once.
// (5) Methods in a class defined only once(Temporarily moses's class type
//		doesn't support method.).
// (6) Reserved identifiers are mot misused.
// And others.
// ---------------------------------Nonsense for coding------------------------
// ����ʹ���﷨�Ƶ��ķ��뷽�����������﷨�����Ĺ����м䣬����һЩ�������������������
// ��Щ������һ��ͳһ�����ƾ���Action Routines������һ�ֽǶȿ��������������ͨ��
// �����ķ�����ʽ��Attribute Grammar���ķ����涨���ˣ��ڽ����﷨������ʱ��Ҫ����
// ��Щ�����������ķ����м̳����Ժ��ۺ����Եĸı䣬�̳����Ծ��ǵ�ǰ�����ڵ�Ӹ��׽ڵ�
// �������ֵܽڵ��õ����ԣ����ۺ�����ֻ���ɵ�ǰ�ڵ�Ϊ���������ṩ�ġ�
//
// moses����һЩ������Ϣ����Ҫ��;�ռ��ģ�moses����������ص����ԣ���Ҳ���Ǻ����ĳЩ�д���Ҫ
// ǰ���ռ�������Ϣ��������ű�scope�ȣ�����Щ��Ϣ�������﷨������������õ��ǵݹ��½���
// �Ĺ�������;�ռ��������ڵ�����´��ݣ���������˷��ű�Symbol Table���Ľṹ��Ϊ
// ȫ�ֿɷ��ʵ����ݽṹ���������ؽڵ㴫�ݵ����⡣����moses������Ƚϼ򵥣����ű����
// һЩ�򵥵�ActionRoutine���Ѿ�����Ҫ���ˡ�
//
// ������Щ����������Ⱥ�˳����������DeclRefExpr�����ȼ���identifier���޶��壬
// û�ж��屨��undefined variable���������˵Ļ���ȡ����������Type Checking.
// ----------------------------------------------------------------------------
//
// -----------------------------------Nonsense for coding------------------------
// Three kinds of languages:
// (1) statically typed: All or almost all checking of types is done as part of
//	   compilation(C, Java, including moses)
// (2) Dynamically typed: Almost all checking of types is done as part of program
//     execution(Scheme, Lisp, python, perl).
// ------------------------------------------------------------------------------
//===---------------------------------------------------------------------===//

// To Do: Ϊ���ڴ�����ϵķ��㣬�Ҵ���ʹ����std::shared_ptr��shared_ptr��Ϊ
// count���2���ֵĿ�����Ϊ���϶������һ��ָ���ֵĿ�����ͬʱ����ʹ����RAII����
// ��ʵ�����ü����������������һ���ֵ�Overhead��
// ������Ҫ�Դ�����о���

#ifndef SEMA_INCLUDE
#define SEMA_INCLUDE
#include <string>
#include <memory>
#include "ASTContext.h"
#include "../Support/Hasing.h"
#include "../Lexer/scanner.h"
#include "../Support/SymbolTable.h"

namespace compiler
{
namespace sema
{
using namespace compiler::ast;
using namespace compiler::lex;
using namespace compiler::Hashing;
using namespace compiler::Support;

/// sema - This implements sematic analysis and AST building for moses.
/// ------------------------nonsense for coding------------------------
/// ����moses���趨���ȶ����ʹ�ã�����Ad-hoc syntax translation����pass
/// �Ϳ����������Ҫ��.
/// ------------------------nonsense for coding------------------------
class Sema
{
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

	// To Do: ����Class scope��Ϊ��֧���Ժ��Class����Ƕ��
	// class B
	// {
	//		public:
	//		class A
	//		{
	//		}
	// }
	// Note: moses��ʱû��ClassǶ�ף�����ClassStackֻ��һ���������Ԫ��
	// ���С�һ������ʾ����Top-Level�ĺ������壬�����������ʾ����Top-Level
	std::vector<std::shared_ptr<ClassSymbol>> ClassStack;

	// To Do: ����Function��Ϊ��֧���Ժ�ĺ����հ�ʵ��
	// func add() -> void
	// {
	//		func sub() -> int {}
	//		return sub;
	// }
	// Note: moses��ʱû��֧�ֱհ�ʵ�֣�����FunctionStackֻ��һ���������Ԫ��
	// ���С�һ������ʾ����Top-Level�ĺ������壬�����������ʾ����Top-Level
	std::vector<std::shared_ptr<FunctionSymbol>> FunctionStack;

  public:
	Sema(ASTContext &Ctx) : Ctx(Ctx) {}

  public:
	// The functions for handling scope.
	// Shit Code!
	// ���������Ψһ����sema���ȡ����ǰ��token��ֻ��ͨ����ȡscan�ĵ�ַ��
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

	// Note: shit code!sema�б����޷���֪��ǰ����λ�á�
	// To Do: ����������ƣ���������һ���������������
	void ActOnTranslationUnitStart();

	void ActOnFunctionDeclStart(std::string name);

	void ActOnFunctionDecl(std::string name, std::shared_ptr<Type> returnType);

	StmtASTPtr ActOnFunctionDeclEnd(std::string name);

	void ActOnParmDecl(std::string name, ParmDeclPtr parm);

	StmtASTPtr ActOnConstDecl(std::string name, std::shared_ptr<Type> type);

	void ActOnVarDecl(std::string name, VarDeclPtr VD);

	void ActOnClassDeclStart(std::string name);
	void ActOnCompoundStmt();

	StmtASTPtr ActOnIfStmt(SourceLocation start, SourceLocation end, ExprASTPtr condtion,
						   StmtASTPtr ThenPart, StmtASTPtr ElsePart);

	std::shared_ptr<Type> ActOnReturnType(const std::string &name) const;

	bool ActOnBreakAndContinueStmt(bool whileContext);

	StmtASTPtr ActOnContinueStmt();

	bool ActOnReturnStmt(std::shared_ptr<Type> type) const;

	StmtASTPtr BuildReturnStmt();

	bool ActOnUnpackDeclElement(std::string name);

	UnpackDeclPtr ActOnUnpackDecl(UnpackDeclPtr unpackDecl, std::shared_ptr<Type> type);

	bool ActOnReturnAnonymous(std::shared_ptr<Type> type) const;

	BinaryPtr ActOnAnonymousTypeVariableAssignment(ExprASTPtr lhs, ExprASTPtr rhs) const;

	// (2) Expression
	ExprASTPtr ActOnConstantExpression();

	std::shared_ptr<Type> ActOnCallExpr(std::string calleeName,
										std::vector<std::shared_ptr<Type>> Args, FunctionDeclPtr &FD);

	ExprASTPtr ActOnUnaryOperator();

	ExprASTPtr ActOnPostfixUnaryOperator();

	ExprASTPtr ActOnBinaryOperator(ExprASTPtr lhs, Token tok, ExprASTPtr rhs);

	VarDeclPtr ActOnDeclRefExpr(std::string name);

	ExprASTPtr ActOnStringLiteral();

	ExprASTPtr ActOnMemberAccessExpr(ExprASTPtr lhs, Token tok);

	ExprASTPtr ActOnExprStmt();

	bool ActOnConditionExpr(std::shared_ptr<Type> type) const;

	std::shared_ptr<Type> ActOnParmDeclUserDefinedType(Token tok) const;

	std::shared_ptr<Type> ActOnVarDeclUserDefinedType(Token tok) const;

	// (3) help method
	bool isInFunctionContext() const { return FunctionStack.size() != 0; }

	/// \brief ע�����ﲻ����ǰ��Inc�ͺ���Inc
	/// (���ߵ�Ψһ������ǰ�߷��صı��ʽ����ֵ�ã������߷��ص�����ֵ��)
	ExprASTPtr ActOnDecOrIncExpr(ExprASTPtr rhs);

	/// \brief ���磺 -10;
	ExprASTPtr ActOnUnarySubExpr(ExprASTPtr rhs);

	/// \brief ���磺!flag
	ExprASTPtr ActOnUnaryExclamatoryExpr(ExprASTPtr rhs);

	// To Do: implement RAII
	class CompoundScopeRAII
	{
	};
	// To Do: implement RAII
	class FunctionScopeRAII
	{
	};

	std::shared_ptr<Scope> getScopeStackBottom() const;

  private:
	std::shared_ptr<Scope> getScopeStackTop() const;

	void errorReport(const std::string &msg) const;

	UnpackDeclPtr unpackDeclTypeChecking(UnpackDeclPtr unpackDecl,
										 std::shared_ptr<Type> initType) const;
};
} // namespace sema
} // namespace compiler
#endif