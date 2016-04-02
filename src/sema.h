//===-----------------------------------sema.h----------------------------===//
// 
// This file defines the Sema class, which performs semantic analysis and 
// builds ASTs. Consult Clang 2.6.
// 
//===---------------------------------------------------------------------===//
#include <string>
#include <memory>
#include "Type.h"

namespace compiler
{
	namespace sema
	{
		using namespace compiler::ast;
		class Symbol;

		/// Scope - A scope is a "transient data structure" that is used while parsing the 
		/// program. It assits with resolving identifiers to the appropriate declaration.
		class Scope
		{
		public:
			enum ScopeKind
			{
				SK_Function,
				SK_Block,
				SK_While,
				SK_Branch,
				SK_Class
			};
		private:
			std::vector<std::shared_ptr<Symbol>> SymbolTable;
			/// SubScopes
			std::vector<std::pair<std::shared_ptr<Scope>, unsigned>> SubScopes;
		public:			
			std::string ScopeName;

			/// The parent scope for this scope. This is null for the translation-unit scope.
			std::shared_ptr<Scope> Parent;

			/// Flags - This contains a set of ScopeFlags, which indicates how the scope 
			/// interrelates with other control flow statements.
			ScopeKind Flags;

			/// Depth - This is the depth of this scope. The translation-unit has depth 0.
			unsigned short Depth;

			/// \brief Add symbol.
			void addDef(std::shared_ptr<Symbol> sym);
			std::shared_ptr<Symbol> Resolve(std::string name);
		public:
			Scope(std::string name, unsigned depth, std::shared_ptr<Scope> paren, ScopeKind kind) :
				ScopeName(name), Parent(paren), Depth(depth), Flags(kind) {}

			/// getFlags - Return the flags for this scope.
			unsigned getFlags() const { return Flags; }
			bool isAnonymous() const { return ScopeName == ""; }
			std::string getScopeName() const { return ScopeName; }
			void setFlags(ScopeKind F) { Flags = F; }
			std::shared_ptr<Scope> getParent() { return Parent; }
		};

		/// \brief symbol - This class is the base class for the identifiers.
		class Symbol
		{
		private:
			std::string lexem;
			std::shared_ptr<Scope> scopeBelongTo;
		public:
			Symbol(std::string lexem, std::shared_ptr<Scope> belongTo) :
				lexem(lexem), scopeBelongTo(belongTo) {}
			virtual std::string getLexem() = 0;
			virtual ~Symbol() {};
		};

		/// \brief VariableSymbol - This class represent variable, 
		/// like 'var a = 10;' or 'var b = true', the 'a' and 'b'.
		class VariableSymbol : public Symbol
		{
		private:
			std::shared_ptr<Type> type;
		public:
			VariableSymbol(std::string lexem, std::shared_ptr<Scope> beongTo,
				std::shared_ptr<Type> type) : Symbol(lexem, beongTo), type(type) {}
		};

		class FunctionSymbol : public Symbol
		{
		private:
			std::shared_ptr<Scope> scope;
		public:
			FunctionSymbol(std::string name, std::shared_ptr<Scope> belongTo, 
				std::shared_ptr<Scope> scope) : 
				Symbol(name, belongTo), scope(scope) {}
		};

		/// \brief ClassSymbol - This class represent class decl symbol.
		/// like 'Class A {}'
		class ClassSymbol : public Symbol
		{
		private:
			std::shared_ptr<Scope> scope;
		public:
			ClassSymbol(std::string name, std::shared_ptr<Scope> belongTo, 
				std::shared_ptr<Scope> scope) : 
				Symbol(name, belongTo), scope(scope) {}
		};

		/// sema - This implements sematic analysis and AST building for moses.
		class Sema
		{
			Sema(const Sema&) = delete;
			void operator=(const Sema&) = delete;
		private:
			std::shared_ptr<Scope> CurScope;

			/// \brief ScopeStack represents scope stack.
			std::vector<std::shared_ptr<Scope>> ScopeStack;

			/// \brief ScopeTree
			std::shared_ptr<Scope> ScopeTreeRoot;
		public:
			Sema() {}
		public:
			// The functions for handling scope.
			std::shared_ptr<Scope> getCurScope() { return CurScope; }
			void PushFunctionScope();
			void PushBlockScope(Scope *BlockScope);
			void PushWhileScope();
			void PushClassScope();

			void PopBlockScope();
			void PopFunctionScope();
			void PopWhileScope();
			void PopClassScope();
		public:
			// The functions for handling Type
			void BuildFunctionType();
			void BuildClassType();
			
			/// \brief  Perform name lookup on the given name, classifying it based on 
			/// the results of name look up and following token.
			/// This routine is used by the parser to resolve identifiers and help direct
			/// parsing. When the identifier cannot be found, this routine will attempt 
			/// to correct the typo and classify based on the resulting name.
			void LookUpName();
		public:
			// Action routines for semantic analysis.
			// (1) Statement
			
			/// \brief Add a new function scope and symbol.
			void ActOnFunctionDecl();
			void ActOnVarDecl();
			void ActOnClassDecl();
			void ActOnCompoundStmt();
			void ActOnIfStmt();
			void ActOnWhileStmt();
			void ActOnBreakStmt();
			void ActOnContinueStmt();
			void ActOnReturnStmt();
			void BuildReturnStmt();
			// (2) Expression
			void ActOnConstantExpression();
			void ActOnCallExpr();
			void ActOnUnaryOperator();
			void ActOnPostfixUnaryOperator();
			void ActOnBinaryOperator();
			void ActOnDeclRefExpr();
			void ActOnStringLiteral();
			void ActOnMemberReferenceExpr();
			void ActOnExprStmt();


			// To Do: implement RAII
			class CompoundScopeRAII
			{};
			// To Do: implement RAII
			class FunctionScopeRAII
			{};

		};
	}
}