//===-----------------------------SymbolTable.h---------------------------===//
//
// This file is used to implement SymbolTable.
// Note: SymbolTable��������;��
// (1) SymbolTable����semantic analysis, ������δ���塢�����ض����Լ����Ͳ�ƥ��
//	   ����
// ���磺	(1)	var num = 10;
// ------------ -----------
//			(2)	var sum = num * 10;		SymbolTable
//|    num	 |    sum	 | 			(3)	if (num > 0)
//------------ ----------- 			(4)	{
//Symbol�д�����ر�����Decl���Լ����͵���Ϣ 			(5)		num = -sum; 			(6)	} 			(7)
//else 			(8)	{ 			(9)		num = sum; 			(10)} 	���ǻ�ͨ�����ű���Ϊ �������� ������
//�����壩-----��ʹ�ã�����ϵ��
//
// (2)
// SymbolTable����IR���ɣ�mosesʹ��visitorģʽ����AST�﷨�����ڱ�������ĳ������
//     ��Referenceʱ�������ű����Ƿ�����Ӧ��IR���ɡ�
// ���磺	(1)	var num = 10;
// ------------ -----------
//			(2)	var sum = num * 10;		SymbolTable
//|    num	 |    sum	 | 			(3)	if (num > 0)	\
///------------ ----------- 			(4)	{				 \
/// 			(5)		num = -sum;	  \	------------------/ 			(6)	}
//| @num = alloca i32 | instr1 			(7)	else
//------------------- 			(8)	{ 			(9)		num = sum; 			(10)}
// ��һ������num��������������һ����num�Ķ��壬������num�����á��ڵ�(1)�У���num�Ķ���
//  ���ǻᴴ��һ��allocaָ��(Ϊnum�����ڴ�)��Ȼ����SymbolTable��ָ���instruction��
//	�ڵ�(2)�У���num����reference���˴����ǻ�����SymbolTable��ͨ��num�鵽instruction��
//	�ڵ�(3)�У���num����reference���˴�����ͬ����ͨ��SymbolTable��ѯ��instruction��
//
// ����������mem��������������һ����mem�Ķ��壬������mem�����á�����ϸ����num��ͬ��
//	��׸����
//===---------------------------------------------------------------------===//
#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H
#include "../IR/Instruction.h"
#include "../IRBuild/CGCall.h"
#include "../Parser/Type.h"
#include "../Parser/ast.h"
#include <memory>
#include <string>

namespace compiler {
namespace Support {
using IRType = compiler::IR::Type;
using namespace compiler::ast;

class Symbol;
class VariableSymbol;
class ClassSymbol;
class FunctionSymbol;
class ScopeSymbol;
/// Scope - A scope is a "transient data structure" that is used while parsing
/// the program. It assits with resolving identifiers to the appropriate
/// declaration.
class Scope {
public:
  // Scope Kind.
  enum ScopeKind {
    SK_Function,
    SK_Block,
    SK_While,
    SK_Branch,
    SK_Class,
    SK_TopLevel
  };

private:
  // A symbol table is a data structure that tracks the current bindings of
  // identifiers
  std::vector<std::shared_ptr<Symbol>> SymbolTable;

  // For function and class.
  std::string ScopeName;

  /// The parent scope for this scope. This is null for the translation-unit
  /// scope.
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
  Scope(std::string name, unsigned depth, std::shared_ptr<Scope> paren,
        ScopeKind kind)
      : ScopeName(name), Parent(paren), Flags(kind), Depth(depth) {}

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

  std::vector<std::shared_ptr<Symbol>> getSymbolTable() const {
    return SymbolTable;
  }

  std::shared_ptr<ClassSymbol> getTheSymbolBelongTo() const { return BelongTo; }

  void setBelongToSymbolForClassScope(std::shared_ptr<ClassSymbol> belongto) {
    BelongTo = belongto;
  }
};

/// \brief symbol - This class is the base class for the identifiers.
class Symbol {
protected:
  using ScopePtr = std::shared_ptr<Scope>;
  std::string Lexem;
  ScopePtr BelongTo;

  /// \brief ����Variable��˵��type��ʾ������������
  /// ����ClassSymbol��˵��type��ʾClass��Ӧ��type.
  /// ����FunctionSymbol��˵��type��ʾFunction�ķ�������.
  std::shared_ptr<Type> type;

public:
  Symbol(std::string lexem, ScopePtr belongTo, std::shared_ptr<Type> type)
      : Lexem(lexem), BelongTo(belongTo), type(type) {}

  std::shared_ptr<Type> getType() const { return type; }

  virtual std::string getLexem() { return Lexem; }

  virtual ~Symbol(){};
};

/// \brief VariableSymbol - This class represent variable,
/// like 'var a = 10;' or 'var b = true', the 'a' and 'b'.
class VariableSymbol final : public Symbol {
  bool IsInitial;
  VarDeclPtr VD;
  IR::AllocaInstPtr allocaInst;

public:
  VariableSymbol(std::string lexem, ScopePtr beongTo,
                 std::shared_ptr<Type> type, bool initial, VarDeclPtr vd)
      : Symbol(lexem, beongTo, type), IsInitial(initial), VD(vd) {}

  ExprASTPtr getInitExpr() const { return VD->getInitExpr(); }
  bool isConstVarDecl() const { return VD->isConst(); }
  VarDeclPtr getDecl() const { return VD; }
  void setInitial(bool initial) { IsInitial = initial; }
  bool isInitial() const { return IsInitial; }

  void setAllocaInst(IR::AllocaInstPtr inst) { allocaInst = inst; }
  IR::AllocaInstPtr getAllocaInst() const { return allocaInst; }
};

/// \brief ParmDeclSymbol - This class represent parm decl.
class ParmDeclSymbol final : public Symbol {
  ParmDeclPtr PD;
  IR::ValPtr allocaInst;

public:
  ParmDeclSymbol(std::string lexem, ScopePtr beongTo,
                 std::shared_ptr<Type> type, bool initial, ParmDeclPtr pd)
      : Symbol(lexem, beongTo, type), PD(pd) {}

  ParmDeclPtr getDecl() const { return PD; }

  void setAllocaInst(IR::ValPtr inst) { allocaInst = inst; }
  IR::ValPtr getAllocaInst() const { return allocaInst; }
};

class FunctionSymbol final : public Symbol {
private:
  ScopePtr scope;
  std::vector<std::shared_ptr<ParmDeclSymbol>> parms;
  FunctionDeclPtr FD;
  IR::FuncPtr FuncAddr;

public:
  FunctionSymbol(std::string name, std::shared_ptr<Type> type,
                 ScopePtr belongTo, ScopePtr scope)
      : Symbol(name, belongTo, type), scope(scope) {}
  std::shared_ptr<Type> getReturnType() { return type; }
  void setReturnType(std::shared_ptr<Type> type) { this->type = type; }

  void addParmVariableSymbol(std::shared_ptr<ParmDeclSymbol> parm) {
    parms.push_back(parm);
  }

  std::shared_ptr<ParmDeclSymbol> operator[](unsigned index) const {
    if (index >= parms.size())
      errorSema("Function parm index out of range");
    return parms[index];
  }

  void setFuncAddr(IR::FuncPtr FuncAddr) { this->FuncAddr = FuncAddr; }
  void setFunctionDeclPointer(FunctionDeclPtr fd) { FD = fd; }

  IR::FuncPtr getFuncAddr() const { return FuncAddr; }
  ScopePtr getScope() const { return scope; }
  unsigned getParmNum() { return parms.size(); }
  FunctionDeclPtr getFuncDeclPointer() const { return FD; }
};

/// \brief ClassSymbol - This class represent class decl symbol.
/// like 'Class A {}'
/// Note: class
///		{
///			var num : int;
///			var flag : bool;
///		};
/// ClassSymbol�л�洢һ�ݶ�����ָ��ָ��UserDefinedType������ASTContext�л��ǻ�
/// �洢һ�ݶ���
class ClassSymbol : public Symbol {
private:
  ScopePtr scope;

public:
  ClassSymbol(std::string name, ScopePtr belongTo, ScopePtr scope)
      : Symbol(name, belongTo,
               std::make_shared<UserDefinedType>(TypeKind::USERDEFIED, name)),
        scope(scope) {}

  /// ---------------------nonsense for coding----------------------------
  /// moses���ýṹ���͵ȼۣ�������Ҫ��pase�Ĺ����У������ռ�UserDefined������
  /// ��SubType.
  /// ---------------------nonsense for coding----------------------------
  void addSubType(std::shared_ptr<Type> subType, std::string name) {
    // Note: ��δ���û�ж�ת���Ƿ�ɹ������жϣ��������⣡
    // ��������type��������UserDefinedType�ģ���ʼ����Ͳ�Ϊ��
    // ͬʱ�ⲿ�Ӵ�����type�������ܽ����޸�Ϊnullptr�������������ǰ�ȫ���÷���
    if (std::shared_ptr<UserDefinedType> UDT =
            std::dynamic_pointer_cast<UserDefinedType>(type)) {
      UDT->addSubType(subType, name);
    }
  }

  /// \brief get the class type
  std::shared_ptr<Type> getType() { return Symbol::getType(); }
};

/// \brief ScopeSymbol - ��ʾһ��scope����Ϊ����õ���scope stack �����Ҫ���͵���ˣ�
/// �ڴ������ɵ�ʱ��ʹ��symbol table��Ϊ����sematic������ʱ�򣬱���������symbol
/// table ��Ϣ�� ���磺
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
/// symbol
/// table����֯��Ҫͨ��symbolʵ�֣���ʱ�޷��������scope����Ϣ�����Զ���һ������
/// ��scope symbol(������Ϊ�˱���symbol��Ϣ)��
class ScopeSymbol final : public Symbol {
private:
  ScopePtr scope;
  bool IsVisitedForIRGen;

public:
  ScopeSymbol(ScopePtr scope, ScopePtr parent)
      : Symbol("", parent, nullptr), scope(scope), IsVisitedForIRGen(false) {}
  bool isVisitedForIRGen() { return IsVisitedForIRGen; }
  void setVisitedFlag() { IsVisitedForIRGen = true; }
  std::shared_ptr<Scope> getScope() const { return scope; }
};
} // namespace Support
} // namespace compiler
#endif