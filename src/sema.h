//===-----------------------------------sema.h----------------------------===//
// 
// This file defines the Sema class, which performs semantic analysis and 
// builds ASTs. Consult Clang 2.6.
// Ad hoc syntax-directed semantic analysis.
//===---------------------------------------------------------------------===//

// To Do: 为了内存管理上的方便，我大量使用了std::shared_ptr，shared_ptr会为
// count添加2个字的开销，为堆上对象添加一个指针字的开销，同时由于使用了RAII机制
// 来实现引用计数的增减，会添加一部分的Overhead。
// 所以需要对代码进行精简！

#ifndef SEMA_INCLUDE
#define SEMA_INCLUDE
#include <string>
#include <memory>
#include "Type.h"
#include "scanner.h"
#include "ast.h"

namespace compiler
{
	namespace sema
	{
		using namespace compiler::ast;
		class Symbol;
		class VariableSymbol;
		class ClassSymbol;
		class FunctionSymbol;

		/// Scope - A scope is a "transient data structure" that is used while parsing the 
		/// program. It assits with resolving identifiers to the appropriate declaration.
		class Scope
		{
		public:
			// Scope Kind.
			enum ScopeKind
			{
				SK_Function,
				SK_Block,
				SK_While,
				SK_Branch,
				SK_Class,
				SK_TopLevel
			};
		private:
			std::vector<std::shared_ptr<Symbol>> SymbolTable;
			/// SubScopes ? ? ?
			std::vector<std::pair<std::shared_ptr<Scope>, unsigned>> SubScopes;

			// For function and class.
			std::string ScopeName;

			/// The parent scope for this scope. This is null for the translation-unit scope.
			std::shared_ptr<Scope> Parent;

			/// Flags - This contains a set of ScopeFlags, which indicates how the scope 
			/// interrelates with other control flow statements.
			ScopeKind Flags;

			/// Depth - This is the depth of this scope. The translation-unit has depth 0.
			unsigned short Depth;

			/// BelongTo - Only for ClassScope.
			/// For Userdefined Type(Class), we need to add subType for class type, so we
			/// using BelongTo to get ClassSymbol for CurScope.
			std::shared_ptr<ClassSymbol> BelongTo;			
		public:
			Scope(std::string name, unsigned depth, std::shared_ptr<Scope> paren, ScopeKind kind) :
				ScopeName(name), Parent(paren), Depth(depth), Flags(kind) {}

			/// getFlags - Return the flags for this scope.
			unsigned getFlags() const { return Flags; }

			/// \brief Add symbol.
			void addDef(std::shared_ptr<Symbol> sym) { SymbolTable.push_back(sym); };

			/// \brief  Perform name lookup on the given name, classifying it based on 
			/// the results of name look up and following token.
			/// This routine is used by the parser to resolve identifiers and help direct
			/// parsing. When the identifier cannot be found, this routine will attempt 
			/// to correct the typo and classify based on the resulting name.
			std::shared_ptr<Symbol> Resolve(std::string name) const;
			
			/// \brief Perform name lookup in current scope.
			std::shared_ptr<Symbol> LookupName(std::string name);

			bool isAnonymous() const { return ScopeName == ""; }	

			std::string getScopeName() const { return ScopeName; }

			void setFlags(ScopeKind F) { Flags = F; }

			unsigned short getDepth() const { return Depth; }

			std::shared_ptr<Scope> getParent() const { return Parent; }

			std::shared_ptr<ClassSymbol> getTheSymbolBelongTo() const { return BelongTo; }

			void setBelongToSymbolForClassScope(std::shared_ptr<ClassSymbol> belongto) 
			{
				BelongTo = belongto;
			}
		};

		/// \brief symbol - This class is the base class for the identifiers.
		class Symbol
		{
		protected:
			std::string Lexem;
			std::shared_ptr<Scope> BelongTo;
			
			/// \brief 对于Variable来说，type表示变量声明类型
			/// 对于ClassSymbol来说，type表示Class对应的type.
			/// 对于FunctionSymbol来说，type表示Function的返回类型.
			std::shared_ptr<Type> type;
		public:
			Symbol(std::string lexem, std::shared_ptr<Scope> belongTo, std::shared_ptr<Type> type) :
				Lexem(lexem), BelongTo(belongTo), type(type) {}
			virtual std::string getLexem() { return Lexem; }
			virtual ~Symbol() {};
		};

		/// \brief VariableSymbol - This class represent variable, 
		/// like 'var a = 10;' or 'var b = true', the 'a' and 'b'.
		class VariableSymbol final: public Symbol
		{
			bool IsInitial;
		public:
			VariableSymbol(std::string lexem, std::shared_ptr<Scope> beongTo,
				std::shared_ptr<Type> type, bool initial) : Symbol(lexem, beongTo, type), IsInitial(initial) 
			{}

			std::shared_ptr<Type> getType() const { return type; }
			void setInitial(bool initial) { IsInitial = initial; }
			bool isInitial() const { return IsInitial; }
		};

		class FunctionSymbol final : public Symbol
		{
		private:
			std::shared_ptr<Scope> scope;
			std::vector<std::shared_ptr<VariableSymbol>> parms;
		public:
			FunctionSymbol(std::string name, std::shared_ptr<Type> type, std::shared_ptr<Scope> belongTo, 
				std::shared_ptr<Scope> scope) : 
				Symbol(name, belongTo, type), scope(scope) {}
			std::shared_ptr<Type> getReturnType() { return type; }
			void setReturnType(std::shared_ptr<Type> type) { this->type = type; }
			void addParmVariableSymbol(std::shared_ptr<VariableSymbol> parm)
			{
				parms.push_back(parm);
			}
			std::shared_ptr<VariableSymbol> operator[] (int index) const
			{
				if (index >= parms.size())
					errorSema("Function parm index out of range");
				return parms[index];
			}
			unsigned getParmNum() { return  parms.size(); }
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
				Symbol(name, belongTo, nullptr), scope(scope)
			{
				type = std::make_shared<UserDefinedType>(TypeKind::USERDEFIED, false, name);
			}
			
			/// ---------------------nonsense for coding----------------------------
			/// moses采用结构类型等价，所以需要在pase的过程中，不断收集UserDefined所含有
			/// 的SubType.
			/// ---------------------nonsense for coding----------------------------
			void addSubType(std::shared_ptr<Type> subType, std::string name) 
			{
				// Note: 这段代码没有对转换是否成功进行判断，存在问题！
				// 但是由于type本来就是UserDefinedType的，初始化后就不为空
				// 同时外部接触不到type，不可能将其修改为nullptr，所以暂且算是安全的用法。
				(dynamic_cast<UserDefinedType*>(type.get()))->addSubType(subType, name); 
			}
			std::shared_ptr<Type> getType() const { return type; }
		};

		/// \brief Represents the results of name lookup.
		/// An instance of the LookupResult class capture the results of a
		/// single name lookup, which can return no result(nothing found),
		/// a single declaration. Use the getKind() method to determine which
		/// of these results occurred for a given lookup.
		///
		/// The result maybe VarDecl, FunctionDec, ClassDecl or ParmDecl.
		class LookupResult
		{
		public:
			/// \brief The king of entity found by name lookup.
			enum LookupKind
			{
				/// \brief No entity found met the criteria.
				NotFound = 0,
				Found
			};
		private:
			DeclStatement* resultDecl;
		public:
			LookupResult(DeclStatement* DS = nullptr) {}

			LookupKind getKind() const;
			/// \brief Determine whether the lookup found something.
			operator bool() const { return getKind() != NotFound; }		
			DeclStatement* getDeclStatement() const { return resultDecl; }
		};

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
			Sema() {}
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

			void ActOnParmDecl(std::string name, std::shared_ptr<Type> type);

			StmtASTPtr ActOnConstDecl(std::string name, std::shared_ptr<Type> type);

			ExprASTPtr ActOnVarDecl(std::string name, std::shared_ptr<Type> type, ExprASTPtr init);

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
				std::vector<std::shared_ptr<Type>> Args);

			ExprASTPtr ActOnUnaryOperator();

			ExprASTPtr ActOnPostfixUnaryOperator();

			ExprASTPtr ActOnBinaryOperator(ExprASTPtr lhs, Token tok, ExprASTPtr rhs);

			std::shared_ptr<Type> ActOnDeclRefExpr(std::string name);

			ExprASTPtr ActOnStringLiteral();

			ExprASTPtr ActOnMemberAccessExpr(ExprASTPtr lhs, Token tok);

			ExprASTPtr ActOnExprStmt();

			bool ActOnConditionExpr(std::shared_ptr<Type> type) const;

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
		private:
			std::shared_ptr<Scope> getScopeStackTop() const;

			void errorReport(const std::string& msg) const;
			
			UnpackDeclPtr unpackDeclTypeChecking(UnpackDeclPtr unpackDecl, 
				std::shared_ptr<Type> initType) const;
		};
	}
}
#endif