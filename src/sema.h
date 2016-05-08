//===-----------------------------------sema.h----------------------------===//
// 
// This file defines the Sema class, which performs semantic analysis and 
// builds ASTs. Consult Clang 2.6.
// Ad hoc syntax-directed semantic analysis.
//===---------------------------------------------------------------------===//

// To Do: Ϊ���ڴ�����ϵķ��㣬�Ҵ���ʹ����std::shared_ptr��shared_ptr��Ϊ
// count���2���ֵĿ�����Ϊ���϶������һ��ָ���ֵĿ�����ͬʱ����ʹ����RAII����
// ��ʵ�����ü����������������һ���ֵ�Overhead��
// ������Ҫ�Դ�����о���

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
			
			/// \brief ����Variable��˵��type��ʾ������������
			/// ����ClassSymbol��˵��type��ʾClass��Ӧ��type.
			/// ����FunctionSymbol��˵��type��ʾFunction�ķ�������.
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
			/// moses���ýṹ���͵ȼۣ�������Ҫ��pase�Ĺ����У������ռ�UserDefined������
			/// ��SubType.
			/// ---------------------nonsense for coding----------------------------
			void addSubType(std::shared_ptr<Type> subType, std::string name) 
			{
				// Note: ��δ���û�ж�ת���Ƿ�ɹ������жϣ��������⣡
				// ��������type��������UserDefinedType�ģ���ʼ����Ͳ�Ϊ��
				// ͬʱ�ⲿ�Ӵ�����type�������ܽ����޸�Ϊnullptr�������������ǰ�ȫ���÷���
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
		/// ����moses���趨���ȶ����ʹ�ã�����Ad-hoc syntax translation����pass
		/// �Ϳ����������Ҫ��.
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
			Sema() {}
		public:
			// The functions for handling scope.
			// Shit Code!
			// ���������Ψһ����sema���ȡ����ǰ��token��ֻ��ͨ����ȡscan�ĵ�ַ��
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

			// Note: shit code!sema�б����޷���֪��ǰ����λ�á�
			// To Do: ����������ƣ���������һ���������������
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

			/// \brief ע�����ﲻ����ǰ��Inc�ͺ���Inc
			/// (���ߵ�Ψһ������ǰ�߷��صı��ʽ����ֵ�ã������߷��ص�����ֵ��)
			ExprASTPtr ActOnDecOrIncExpr(ExprASTPtr rhs);

			/// \brief ���磺 -10;
			ExprASTPtr ActOnUnarySubExpr(ExprASTPtr rhs);

			/// \brief ���磺!flag
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