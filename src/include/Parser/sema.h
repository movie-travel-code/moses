//===-----------------------------------sema.h----------------------------===//
// 
// This file defines the Sema class, which performs semantic analysis and 
// builds ASTs. Consult Clang 2.6.
// Ad hoc syntax-directed semantic analysis.
//
// Checks of many kinds.
// (1) All identifiers are declared.
// (2) Types (经过语义分析之后，语法树上的任意表达式都有其对应类型， 即fully typed)
// (3) Inheritance relationsships(Temporarily moses's class type doesn't have
//		inheritance).
// (4) classes defined only once.
// (5) Methods in a class defined only once(Temporarily moses's class type
//		doesn't support method.).
// (6) Reserved identifiers are mot misused.
// And others.
// ---------------------------------Nonsense for coding------------------------
// 我们使用语法制导的翻译方法，就是在语法分析的过程中间，调用一些函数来进行语义分析，
// 这些函数有一个统一的名称就是Action Routines。从另一种角度看语义分析，就是通过
// 属性文法的形式，Attribute Grammar在文法层面定义了，在进行语法分析的时候要进行
// 哪些操作，属性文法中有继承属性和综合属性的改变，继承属性就是当前分析节点从父亲节点
// 和左手兄弟节点获得的属性，而综合属性只是由当前节点为根的子树提供的。
// 
// moses中有一些属性信息是需要沿途收集的（moses是上下文相关的语言），也就是后面的某些判错需要
// 前面收集到的信息（例如符号表，scope等）。这些信息可以在语法分析（这里采用的是递归下降）
// 的过程中沿途收集，挨个节点的向下传递，这里采用了符号表（Symbol Table）的结构作为
// 全局可访问的数据结构，避免了沿节点传递的问题。还好moses的语义比较简单，符号表加上
// 一些简单的ActionRoutine就已经满足要求了。
// 
// 并且有些语义分析有先后顺序，例如遇到DeclRefExpr，首先检查该identifier有无定义，
// 没有定义报错“undefined variable”，定义了的话获取其类型再做Type Checking.
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

// To Do: 为了内存管理上的方便，我大量使用了std::shared_ptr，shared_ptr会为
// count添加2个字的开销，为堆上对象添加一个指针字的开销，同时由于使用了RAII机制
// 来实现引用计数的增减，会添加一部分的Overhead。
// 所以需要对代码进行精简！

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
		/// 由于moses的设定，先定义后使用，所以Ad-hoc syntax translation单遍pass
		/// 就可以满足这个要求.
		/// ------------------------nonsense for coding------------------------
		class Sema
		{
			Sema(const Sema&) = delete;
			void operator=(const Sema&) = delete;
			parse::Scanner* scan;
			ASTContext &Ctx;
		private:
			std::shared_ptr<Scope> CurScope;

			/// \brief ScopeStack represents scope stack.
			std::vector<std::shared_ptr<Scope>> ScopeStack;

			/// \brief ScopeTree
			std::shared_ptr<Scope> ScopeTreeRoot;

			// To Do: 设置Class scope，为了支持以后的Class定义嵌套
			// class B
			// {
			//		public:
			//		class A
			//		{
			//		}
			// }
			// Note: moses暂时没有Class嵌套，所以ClassStack只有一个或者零个元素
			// 其中“一个”表示处于Top-Level的函数定义，而“零个”表示处于Top-Level
			std::vector<std::shared_ptr<ClassSymbol>> ClassStack;

			// To Do: 设置Function，为了支持以后的函数闭包实现
			// func add() -> void 
			// {
			//		func sub() -> int {}
			//		return sub;
			// }
			// Note: moses暂时没有支持闭包实现，所以FunctionStack只有一个或者零个元素
			// 其中“一个”表示处于Top-Level的函数定义，而“零个”表示处于Top-Level
			std::vector<std::shared_ptr<FunctionSymbol>> FunctionStack;
		public:
			Sema(ASTContext &Ctx) : Ctx(Ctx) {}
		public:
			// The functions for handling scope.
			// Shit Code!
			// 这个函数的唯一作用sema想获取到当前的token，只能通过获取scan的地址。
			void getScannerPointer(parse::Scanner* scan) { this->scan = scan; };
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

			// Note: shit code!sema中报错无法获知当前报错位置。
			// To Do: 调整报错机制，单独定义一个报错引擎出来。
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

			std::shared_ptr<Type> ActOnReturnType(const std::string& name) const;

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

			/// \brief 注意这里不区分前置Inc和后置Inc
			/// (两者的唯一区别是前者返回的表达式是左值得，而后者返回的是右值的)
			ExprASTPtr ActOnDecOrIncExpr(ExprASTPtr rhs);

			/// \brief 例如： -10;
			ExprASTPtr ActOnUnarySubExpr(ExprASTPtr rhs);

			/// \brief 例如：!flag
			ExprASTPtr ActOnUnaryExclamatoryExpr(ExprASTPtr rhs);

			// To Do: implement RAII
			class CompoundScopeRAII
			{};
			// To Do: implement RAII
			class FunctionScopeRAII
			{};

			std::shared_ptr<Scope> getScopeStackBottom() const;
		private:
			std::shared_ptr<Scope> getScopeStackTop() const;

			void errorReport(const std::string& msg) const;
			
			UnpackDeclPtr unpackDeclTypeChecking(UnpackDeclPtr unpackDecl, 
				std::shared_ptr<Type> initType) const;
		};
	}
}
#endif