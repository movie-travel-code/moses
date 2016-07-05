//===-----------------------------SymbolTable.h---------------------------===//
//
// This file is used to implement SymbolTable.
// Note: SymbolTable有两个用途：
// (1) SymbolTable用于semantic analysis, 检查符号未定义、符号重定义以及类型不匹配
//	   错误。
// 例如：	(1)	var num = 10;							 ------------ -----------
//			(2)	var sum = num * 10;		SymbolTable		|    num	 |    sum	 |
//			(3)	if (num > 0)							 ------------ -----------
//			(4)	{				 		Symbol中存有相关变量的Decl，以及类型等信息
//			(5)		num = -sum;			
//			(6)	}				   
//			(7)	else			
//			(8)	{
//			(9)		num = sum;
//			(10)}
//	我们会通过符号表作为 “桥梁” 建立起 （定义）-----（使用）的联系。
//
// (2) SymbolTable用于IR生成，moses使用visitor模式遍历AST语法树，在遍历到对某个变量
//     的Reference时，检查符号表中是否有相应的IR生成。
// 例如：	(1)	var num = 10;							 ------------ -----------
//			(2)	var sum = num * 10;		SymbolTable		|    num	 |    sum	 |
//			(3)	if (num > 0)	\						/------------ -----------
//			(4)	{				 \					   /
//			(5)		num = -sum;	  \	------------------/
//			(6)	}				   | @num = alloca i32 | instr1
//			(7)	else				-------------------
//			(8)	{
//			(9)		num = sum;
//			(10)}
// （一）对于num，上述代码中有一处对num的定义，两处对num的引用。在第(1)行，对num的定义
//  我们会创建一条alloca指令(为num分配内存)，然后在SymbolTable中指向该instruction。
//	在第(2)行，对num进行reference，此处我们会索引SymbolTable，通过num查到instruction。
//	在第(3)行，对num进行reference，此处我们同样会通过SymbolTable查询到instruction。
//
// （二）对于mem，上述代码中有一处对mem的定义，两处对mem的引用。具体细节与num相同，
//	不赘述。
//===---------------------------------------------------------------------===//
#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H
#include <string>
#include <memory>
#include "../Parser/ast.h"
#include "../Parser/Type.h"
#include "../IR/Instruction.h"
namespace compiler
{
	namespace Support
	{
		using IRType = compiler::IR::Type;
		using namespace compiler::ast;

		class Symbol;
		class VariableSymbol;
		class ClassSymbol;
		class FunctionSymbol;
		class ScopeSymbol;
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

			// A symbol table is a data structure that tracks the current bindings of identifiers
			std::vector<std::shared_ptr<Symbol>> SymbolTable;

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
			// std::shared_ptr<Symbol> LookupName(std::string name);
			std::shared_ptr<Symbol> CheckWhetherInCurScope(std::string name);

			bool isAnonymous() const { return ScopeName == ""; }

			std::string getScopeName() const { return ScopeName; }

			void setFlags(ScopeKind F) { Flags = F; }

			unsigned short getDepth() const { return Depth; }

			std::shared_ptr<Scope> getParent() const { return Parent; }

			std::vector<std::shared_ptr<Symbol>> getSymbolTable() const { return SymbolTable; }

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
			using ScopePtr = std::shared_ptr<Scope>;
			std::string Lexem;
			ScopePtr BelongTo;

			/// \brief 对于Variable来说，type表示变量声明类型
			/// 对于ClassSymbol来说，type表示Class对应的type.
			/// 对于FunctionSymbol来说，type表示Function的返回类型.
			std::shared_ptr<Type> type;
		public:
			Symbol(std::string lexem, ScopePtr belongTo, std::shared_ptr<Type> type) :
				Lexem(lexem), BelongTo(belongTo), type(type) {}

			std::shared_ptr<Type> getType() const { return type; }

			virtual std::string getLexem() { return Lexem; }

			virtual ~Symbol() {};
		};

		/// \brief VariableSymbol - This class represent variable, 
		/// like 'var a = 10;' or 'var b = true', the 'a' and 'b'.
		class VariableSymbol final : public Symbol
		{
			bool IsInitial;
			VarDeclPtr VD;
			IR::AllocaInstPtr allocaInst;
		public:
			VariableSymbol(std::string lexem, ScopePtr beongTo, std::shared_ptr<Type> type,
				bool initial, VarDeclPtr vd) :
				Symbol(lexem, beongTo, type), IsInitial(initial), VD(vd)
			{}		

			ExprASTPtr getInitExpr() const { return VD->getInitExpr(); }
			bool isConstVarDecl() const { return VD->isConst(); }
			VarDeclPtr getDecl() const { return VD; }
			void setInitial(bool initial) { IsInitial = initial; }
			bool isInitial() const { return IsInitial; }

			void setAllocaInst(IR::AllocaInstPtr inst) { allocaInst = inst; }
			IR::AllocaInstPtr getAllocaInst() const { return allocaInst; }
		};

		/// \brief ParmDeclSymbol - This class represent parm decl.
		class ParmDeclSymbol final : public Symbol
		{
			ParmDeclPtr PD;
			IR::AllocaInstPtr allocaInst;
		public:
			ParmDeclSymbol(std::string lexem, ScopePtr beongTo,
				std::shared_ptr<Type> type, bool initial, ParmDeclPtr pd) :
				Symbol(lexem, beongTo, type), PD(pd)
			{}

			ParmDeclPtr getDecl() const { return PD; }

			void setAllocaInst(IR::AllocaInstPtr inst) { allocaInst = inst; }
			IR::AllocaInstPtr getAllocaInst() const { return allocaInst; }
		};

		class FunctionSymbol final : public Symbol
		{
		private:
			ScopePtr scope;
			std::vector<std::shared_ptr<ParmDeclSymbol>> parms;
			FunctionDeclPtr FD;
		public:
			FunctionSymbol(std::string name, std::shared_ptr<Type> type, ScopePtr belongTo, ScopePtr scope) :
				Symbol(name, belongTo, type), scope(scope) {}
			std::shared_ptr<Type> getReturnType() { return type; }
			void setReturnType(std::shared_ptr<Type> type) { this->type = type; }

			void addParmVariableSymbol(std::shared_ptr<ParmDeclSymbol> parm)
			{
				parms.push_back(parm);
			}

			std::shared_ptr<ParmDeclSymbol> operator[] (unsigned index) const
			{
				if (index >= parms.size())
					errorSema("Function parm index out of range");
				return parms[index];
			}

			void setFunctionDeclPointer(FunctionDeclPtr fd)
			{
				FD = fd;
			}
			ScopePtr getScope() const { return scope; }
			unsigned getParmNum() { return  parms.size(); }
			FunctionDeclPtr getFuncDeclPointer() const { return FD; }
		};

		/// \brief ClassSymbol - This class represent class decl symbol.
		/// like 'Class A {}'
		/// Note: class
		///		{
		///			var num : int;
		///			var flag : bool;
		///		};
		/// ClassSymbol中会存储一份儿智能指针指向UserDefinedType，但是ASTContext中还是会
		/// 存储一份儿。
		class ClassSymbol : public Symbol
		{
		private:
			ScopePtr scope;
		public:
			ClassSymbol(std::string name, ScopePtr belongTo, ScopePtr scope) :
				Symbol(name, belongTo, std::make_shared<UserDefinedType>(TypeKind::USERDEFIED, name)),
				scope(scope)
			{}

			/// ---------------------nonsense for coding----------------------------
			/// moses采用结构类型等价，所以需要在pase的过程中，不断收集UserDefined所含有
			/// 的SubType.
			/// ---------------------nonsense for coding----------------------------
			void addSubType(std::shared_ptr<Type> subType, std::string name)
			{
				// Note: 这段代码没有对转换是否成功进行判断，存在问题！
				// 但是由于type本来就是UserDefinedType的，初始化后就不为空
				// 同时外部接触不到type，不可能将其修改为nullptr，所以暂且算是安全的用法。
				if (std::shared_ptr<UserDefinedType> UDT = std::dynamic_pointer_cast<UserDefinedType>(type))
				{
					UDT->addSubType(subType, name);
				}
			}

			/// \brief get the class type
			std::shared_ptr<Type> getType() { return Symbol::getType(); }
		};

		/// \brief ScopeSymbol - 表示一个scope，因为这里得到的scope stack 结果还要传送到后端，
		/// 在代码生成的时候使用symbol table，为了在sematic结束的时候，保留完整的symbol table
		/// 信息。
		/// 例如：
		///		func add(lhs : int, rhs : int) -> int
		///		{
		///			if (lhs > rhs)
		///			{
		///				var flag = 14;
		///				return lhs + rhs + flag;
		///			}
		///			var num = 44;
		///			return num + rhs;
		///		}
		/// symbol table的组织主要通过symbol实现，此时无法留存匿名scope的信息，所以定义一个匿名
		/// 的scope symbol(仅仅是为了保存symbol信息)。
		class ScopeSymbol final : public Symbol
		{
		private:
			ScopePtr scope;
			bool IsVisitedForIRGen;
		public:
			ScopeSymbol(ScopePtr scope, ScopePtr parent) : 
				Symbol("", parent, nullptr), scope(scope), IsVisitedForIRGen(false)
			{}
			bool isVisitedForIRGen() { return IsVisitedForIRGen; }
			void setVisitedFlag() { IsVisitedForIRGen = true; }
			std::shared_ptr<Scope> getScope() const { return scope; }
		};
	}
}
#endif