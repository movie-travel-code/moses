//===----------------------------------BasicBlock.h-----------------------===//
//
// This file contains the declaration of the BasicBlock class, which represents
// a single basic block in the vm.
//
// Note that well formed basic blocks are formed of a list of instructions
// followed by a single TerminatorInst instruction. TerminatorInst's may not
// occur in the middle of basic blocks, and must terminate the blocks.
//
// This code allows malformed basix blocks to occur, because it may be
// useful in the intermediate stage modification to a program.
//
//===---------------------------------------------------------------------===//
#pragma once
#include "Function.h"
#include "Instruction.h"
#include <memory>
#include <string>

namespace IR {
class TerminatorInst;
/// \brief moses IR(LLVM) Basic Block Representation
/// This represents a single basic block in moses IR. A basic block is simply a
/// container of instructions that execute sequentially. Basic blocks are Values
/// because they are referenced by instructions such as branches. The type of a
/// BasicBlock is "Type::LabelTy" because the basic block represents a label to
/// which a branch can jump.
class BasicBlock : public Value {
private:
  // Instruction List
  std::list<std::shared_ptr<Instruction>> InstList;
  std::shared_ptr<Function> Parent;

  BasicBlock(const BasicBlock &) = delete;
  void operator=(const BasicBlock &) = delete;

public:
  /// BasicBlock ctor - If the function parameter is specified, the basic block
  /// is automatically inserted at the end of the function.
  BasicBlock(const std::string &Name, std::shared_ptr<Function> Parent,
             std::shared_ptr<BasicBlock> InsertBefore = nullptr);
  ~BasicBlock() {}

  /// \brief Creates a new BasicBlock.
  ///
  /// If the Parent parameter is specified, the basic block is automatically
  /// inserted at either the end of the function (if InsertBefore is 0), or
  /// before the specified basic block.
  static std::shared_ptr<BasicBlock>
  Create(const std::string &Name, std::shared_ptr<Function>,
         std::shared_ptr<BasicBlock> = nullptr);
  bool RemoveInst(const Value *val);
  void setParent(std::shared_ptr<Function> parent) { Parent = parent; }
  // Specialize setName to take care of symbol table majik
  virtual void setName(const std::string &name) override { Name = name; }
  // getParent - Return the enclosing method, or null if none
  std::shared_ptr<Function> getParent() { return Parent; }

  std::vector<std::shared_ptr<BasicBlock>> getPredecessors() const;

  /// \brief Remove 'this' from the containing function.
  /// \returns the element after the erased one.
  std::shared_ptr<BasicBlock> removeFromParent();

  /// removePredecessor - This method is used to notify a BasicBlock that the
  /// specified predicesoor of the block is no longer able to reach it. This is
  /// actully not used to update the Predecessor list, but it is actully used to
  /// update the PHI nodes that reside in the block. Note that this should be
  /// called while the predecessor still refers to this block.
  void removePredecessor(std::shared_ptr<BasicBlock> Pred);

  /// splitBasicBlock - This splits a basic block into two the specified
  /// instruction. Note that all instructions BEFORE the specified iterator
  /// stay as part of the original basic block, an unconditional branch is
  /// added to the new BB, and the rest of the instructions in the BB are
  /// moved to the new BB, including the old terminator. The newly formed
  /// BasicBlock is returned.
  std::shared_ptr<BasicBlock> splitBasicBlock(unsigned index,
                                              std::string BBName = "");

  /// getTerminator() - If this is a well formed basic block, the this returns
  /// a pointer to the terminator instruction. It it is not, then you get a
  /// null pointer back.
  std::shared_ptr<TerminatorInst> getTerminator();

  //===--------------------------------------------------------------------===//
  // Instruction iterator methods
  //===--------------------------------------------------------------------===//
  std::list<std::shared_ptr<Instruction>> &getInstList() { return InstList; }
  std::list<std::shared_ptr<Instruction>>::iterator
  Insert(Iterator InsertP, std::shared_ptr<Instruction> I);
  void Push(std::shared_ptr<Instruction> I) { InstList.push_back(I); }
  Iterator getIterator(std::shared_ptr<Instruction> I);
  std::list<std::shared_ptr<Instruction>>::iterator begin();
  std::list<std::shared_ptr<Instruction>>::iterator end();
  // methods for support type inquiry thorough isa, cast, and dyn_cast
  static bool classof(std::shared_ptr<Value> V) {
    return V->getValueType() == Value::ValueTy::BasicBlockVal;
  }

  /// \brief Print the BasicBlock.
  void Print(std::ostringstream &out) override;
};
} // namespace IR
