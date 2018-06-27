//===---------------------------------Function.h--------------------------===//
//
// This file contains the declaration of the Function class, which represents
// a single function/procedure in moses IR.
//
// A function basically consists of a list of basic blocks, a list of arguments,
// and a symbol table.
//
//===---------------------------------------------------------------------===//
#ifndef MOSES_IR_FUNCTION_H
#define MOSES_IR_FUNCTION_H
#include "ConstantAndGlobal.h"
#include "ValueSymbolTable.h"
#include <list>
#include <string>


namespace compiler {
namespace IR {
class BasickBlock;
/// \brief moses IR(LLVM) Argument representation.
///
/// This class represents an incoming formal argument to a Function. A formal
/// argument, since it is 'formal', does not contain an actual value but instead
/// represents the type, arguement number, and attributes of an argument for a
/// specific function. When used in the body of said funciton, the argument of
/// course represents the value of the actual argument that the function was
/// called with.
class Argument : public Value {
  FuncPtr Parent;
  void setParent(FuncPtr parent);

public:
  /// Argument ctor - If Function argument is specified, this argument is
  /// inserted at the end of the argument list for the function.
  Argument(TyPtr Ty, std::string Name = "", FuncPtr F = nullptr);
  void setType(TyPtr ty) { this->Ty = Ty; }
  FuncPtr getParent() { return Parent; }
  static bool classof(ValPtr V) {
    return V->getValueType() == Value::ValueTy::ArgumentVal;
  }

  /// \brief Print the Argument.
  void Print(std::ostringstream &out);
};

class Function : public GlobalValue {
private:
  // Important things that make up a function!
  TyPtr ReturnType;
  FuncTypePtr FunctionTy;
  std::list<BBPtr> BasicBlocks;
  std::vector<ArgPtr> Arguments;

public:
  Function(FuncTypePtr Ty, std::string Name, std::vector<std::string> Names);

  static FuncPtr create(FuncTypePtr Ty, std::string Name,
                        std::vector<std::string> Names);
  ArgPtr operator[](unsigned index) const;
  /// \brief Set argument name and type.
  void setArgumentInfo(unsigned index, std::string name);

  void addBB(BBPtr B) { BasicBlocks.push_back(B); }

  ArgPtr getArg(unsigned index) const { return (*this)[index]; }
  TyPtr getReturnType() const;
  TyPtr getFunctionType() const { return FunctionTy; }

  /// Get the underlying elements of the Function... the basic block list is
  /// empty for external functions.
  std::vector<ArgPtr> &getArgumentList() { return Arguments; }
  std::list<BBPtr> &getBasicBlockList() { return BasicBlocks; }
  ArgPtr operator[](unsigned index) { return Arguments[index]; }
  const BBPtr &getEntryBlock() const { return BasicBlocks.front(); }

  /// Determine if the function is known not to recurse, directly or
  /// indirectly.
  bool doesNotRecurse() const { return true; }
  void dropAllReferences() {}
  bool doesNotAccessMemory(unsigned n) const { return true; }
  void setDoseNotAccessMemory(unsigned n) {}
  bool onlyReadsMemory(unsigned n) const { return true; }
  void setOnlyReadsMemory(unsigned n) {}
  /// Optimize this function for minimum size (-Oz).
  bool optForMinSize() const { return true; }
  /// Optimize this function for size (-Os) or minimum size (-Oz).
  bool optForSize() const { return true; }

  static bool classof(ValPtr V) {
    return V->getValueType() == Value::ValueTy::FunctionVal;
  }
  /// \brief Print the function info.
  void Print(std::ostringstream &out);
};

// Intrinsic���ڻ��ܼ�ª
class Intrinsic : public GlobalValue {
  std::vector<std::string> Names;

public:
  Intrinsic(std::string IntrName, std::vector<std::string> ArgNames)
      : GlobalValue(nullptr, ValueTy::FunctionVal, IntrName) {
    for (auto item : ArgNames) {
      Names.push_back(item);
    }
  }
  void Print(std::ostringstream &out);
};
} // namespace IR
} // namespace compiler

#endif
