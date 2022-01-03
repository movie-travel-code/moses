//===---------------------------------Function.h--------------------------===//
//
// This file contains the declaration of the Function class, which represents
// a single function/procedure in moses IR.
//
// A function basically consists of a list of basic blocks, a list of arguments,
// and a symbol table.
//
//===---------------------------------------------------------------------===//
#pragma once
#include "ConstantAndGlobal.h"
#include "ValueSymbolTable.h"
#include <list>
#include <string>

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
  std::shared_ptr<Function> Parent;
  void setParent(std::shared_ptr<Function> parent);

public:
  /// Argument ctor - If Function argument is specified, this argument is
  /// inserted at the end of the argument list for the function.
  Argument(TyPtr Ty, const std::string &Name = "",
           std::shared_ptr<Function> F = nullptr);
  void setType(TyPtr Ty) { this->Ty = Ty; }
  std::shared_ptr<Function> getParent() { return Parent; }
  static bool classof(std::shared_ptr<Value> V) {
    return V->getValueType() == Value::ValueTy::ArgumentVal;
  }

  /// \brief Print the Argument.
  void Print(std::ostringstream &out);
};

class Function : public GlobalValue {
private:
  // Important things that make up a function!
  TyPtr ReturnType;
  std::shared_ptr<FunctionType> FunctionTy;
  std::list<std::shared_ptr<BasicBlock>> BasicBlocks;
  std::vector<std::shared_ptr<Argument>> Arguments;

public:
  Function(std::shared_ptr<FunctionType> Ty, const std::string &Name,
           std::vector<std::string> Names);

  static std::shared_ptr<Function> create(std::shared_ptr<FunctionType> Ty,
                                          const std::string &Name,
                                          std::vector<std::string> Names);
  std::shared_ptr<Argument> operator[](unsigned index) const;
  /// \brief Set argument name and type.
  void setArgumentInfo(unsigned index, const std::string &name);

  void addBB(std::shared_ptr<BasicBlock> B) { BasicBlocks.push_back(B); }

  std::shared_ptr<Argument> getArg(unsigned index) const {
    return (*this)[index];
  }
  TyPtr getReturnType() const;
  TyPtr getFunctionType() const { return FunctionTy; }

  /// Get the underlying elements of the Function... the basic block list is
  /// empty for external functions.
  std::vector<std::shared_ptr<Argument>> &getArgumentList() {
    return Arguments;
  }
  std::list<std::shared_ptr<BasicBlock>> &getBasicBlockList() {
    return BasicBlocks;
  }
  std::shared_ptr<Argument> operator[](unsigned index) {
    return Arguments[index];
  }
  const std::shared_ptr<BasicBlock> &getEntryBlock() const {
    return BasicBlocks.front();
  }

  /// Determine if the function is known not to recurse, directly or
  /// indirectly.
  bool doesNotRecurse() const { return true; }
  void dropAllReferences() {}
  bool doesNotAccessMemory([[maybe_unused]] unsigned n) const { return true; }
  void setDoseNotAccessMemory([[maybe_unused]] unsigned n) {}
  bool onlyReadsMemory([[maybe_unused]] unsigned n) const { return true; }
  void setOnlyReadsMemory([[maybe_unused]] unsigned n) {}
  /// Optimize this function for minimum size (-Oz).
  bool optForMinSize() const { return true; }
  /// Optimize this function for size (-Os) or minimum size (-Oz).
  bool optForSize() const { return true; }

  static bool classof(std::shared_ptr<Value> V) {
    return V->getValueType() == Value::ValueTy::FunctionVal;
  }
  /// \brief Print the function info.
  void Print(std::ostringstream &out);
};

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
