//===-----------------------------SymbolTable.h---------------------------===//
//
// This file is used to implement SymbolTable.
//
//===---------------------------------------------------------------------===//
#pragma once
#include "IR/Instruction.h"
#include "IRBuild/CGCall.h"
#include "Parser/Type.h"
#include "Parser/ast.h"
#include <memory>
#include <string>

namespace Support {
using IRType = IR::Type;
using namespace ast;
using namespace IR;

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
  enum class ScopeKind {
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
  Scope(const std::string &name, unsigned depth, std::shared_ptr<Scope> paren,
        ScopeKind kind)
      : ScopeName(name), Parent(paren), Flags(kind), Depth(depth) {}

  /// getFlags - Return the flags for this scope.
  ScopeKind getFlags() const { return Flags; }

  /// \brief Add symbol.
  void addDef(std::shared_ptr<Symbol> sym) { SymbolTable.push_back(sym); };

  /// \brief  Perform name lookup on the given name, classifying it based on
  /// the results of name look up and following token.
  /// This routine is used by the parser to resolve identifiers and help direct
  /// parsing. When the identifier cannot be found, this routine will attempt
  /// to correct the typo and classify based on the resulting name.
  std::shared_ptr<Symbol> Resolve(const std::string &name) const;

  /// \brief Perform name lookup in current scope.
  // std::shared_ptr<Symbol> LookupName(std::string name);
  std::shared_ptr<Symbol> CheckWhetherInCurScope(const std::string &name);

  bool isAnonymous() const { return ScopeName == ""; }

  const std::string &getScopeName() const { return ScopeName; }

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

  std::shared_ptr<ASTType> type;

public:
  Symbol(const std::string &lexem, ScopePtr belongTo,
         std::shared_ptr<ASTType> type)
      : Lexem(lexem), BelongTo(belongTo), type(type) {}

  std::shared_ptr<ASTType> getType() const { return type; }

  virtual const std::string &getLexem() { return Lexem; }

  virtual ~Symbol(){};
};

/// \brief VariableSymbol - This class represent variable,
/// like 'var a = 10;' or 'var b = true', the 'a' and 'b'.
class VariableSymbol final : public Symbol {
  bool IsInitial;
  VarDeclPtr VD;
  std::shared_ptr<AllocaInst> allocaInst;

public:
  VariableSymbol(const std::string &lexem, ScopePtr beongTo,
                 std::shared_ptr<ASTType> type, bool initial, VarDeclPtr vd)
      : Symbol(lexem, beongTo, type), IsInitial(initial), VD(vd) {}

  ExprASTPtr getInitExpr() const { return VD->getInitExpr(); }
  bool isConstVarDecl() const { return VD->isConst(); }
  VarDeclPtr getDecl() const { return VD; }
  void setInitial(bool initial) { IsInitial = initial; }
  bool isInitial() const { return IsInitial; }

  void setAllocaInst(std::shared_ptr<AllocaInst> inst) { allocaInst = inst; }
  std::shared_ptr<AllocaInst> getAllocaInst() const { return allocaInst; }
};

/// \brief ParmDeclSymbol - This class represent parm decl.
class ParmDeclSymbol final : public Symbol {
  ParmDeclPtr PD;
  std::shared_ptr<Value> allocaInst;

public:
  ParmDeclSymbol(const std::string &lexem, ScopePtr beongTo,
                 std::shared_ptr<ASTType> type, [[maybe_unused]] bool initial,
                 ParmDeclPtr pd)
      : Symbol(lexem, beongTo, type), PD(pd) {}

  ParmDeclPtr getDecl() const { return PD; }

  void setAllocaInst(std::shared_ptr<Value> inst) { allocaInst = inst; }
  std::shared_ptr<Value> getAllocaInst() const { return allocaInst; }
};

class FunctionSymbol final : public Symbol {
private:
  ScopePtr scope;
  std::vector<std::shared_ptr<ParmDeclSymbol>> parms;
  FunctionDeclPtr FD;
  std::shared_ptr<IR::Function> FuncAddr;

public:
  FunctionSymbol(const std::string &name, std::shared_ptr<ASTType> type,
                 ScopePtr belongTo, ScopePtr scope)
      : Symbol(name, belongTo, type), scope(scope) {}
  std::shared_ptr<ASTType> getReturnType() { return type; }
  void setReturnType(std::shared_ptr<ASTType> type) { this->type = type; }

  void addParmVariableSymbol(std::shared_ptr<ParmDeclSymbol> parm) {
    parms.push_back(parm);
  }

  std::shared_ptr<ParmDeclSymbol> operator[](unsigned index) const {
    if (index >= parms.size())
      errorSema("Function parm index out of range");
    return parms[index];
  }

  void setFuncAddr(std::shared_ptr<IR::Function> FuncAddr) {
    this->FuncAddr = FuncAddr;
  }
  void setFunctionDeclPointer(FunctionDeclPtr fd) { FD = fd; }

  std::shared_ptr<IR::Function> getFuncAddr() const { return FuncAddr; }
  ScopePtr getScope() const { return scope; }
  std::size_t getParmNum() { return parms.size(); }
  FunctionDeclPtr getFuncDeclPointer() const { return FD; }
};

/// \brief ClassSymbol - This class represent class decl symbol.
/// like 'Class A {}'
/// Note: class
///		{
///			var num : int;
///			var flag : bool;
///		};
class ClassSymbol : public Symbol {
private:
  ScopePtr scope;

public:
  ClassSymbol(const std::string &name, ScopePtr belongTo, ScopePtr scope)
      : Symbol(name, belongTo,
               std::make_shared<UserDefinedType>(TypeKind::USERDEFIED, name)),
        scope(scope) {}

  void addSubType(std::shared_ptr<ASTType> subType, std::string name) {
    if (std::shared_ptr<UserDefinedType> UDT =
            std::dynamic_pointer_cast<UserDefinedType>(type)) {
      UDT->addSubType(subType, name);
    }
  }

  /// \brief get the class type
  std::shared_ptr<ASTType> getType() { return Symbol::getType(); }
};

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
