//===-----------------------------BasicBlock.cpp---------------------------===//
//
// This file implements the BasicBlock class for the IR library.
//
//===----------------------------------------------------------------------===//
#include "IR/BasicBlock.h"
#include "IR/ConstantAndGlobal.h"
#include "IR/IRType.h"
#include "IR/Instruction.h"

using namespace IR;

BasicBlock::BasicBlock(const std::string &Name, std::shared_ptr<Function> Parent,
                       [[maybe_unused]] std::shared_ptr<BasicBlock> InsertBefore)
    : Value(std::make_shared<Type>(Type::TypeID::LabelTy),
            Value::ValueTy::BasicBlockVal, Name),
      Parent(Parent) {}

std::shared_ptr<BasicBlock> BasicBlock::Create(const std::string &Name, std::shared_ptr<Function> Parent,
                         std::shared_ptr<BasicBlock> InsertBefore) {
  return std::make_shared<BasicBlock>(Name, Parent, InsertBefore);
}

// Get the predecessors of this basic block.
std::vector<std::shared_ptr<BasicBlock>> BasicBlock::getPredecessors() const {
  // (1) get the use list.
  auto Uses = getUses();
  std::vector<std::shared_ptr<BasicBlock>> Predecessors;
  Predecessors.reserve(Uses.size());
  // (2) get the use's father
  for (const auto &item : Uses) {
    const auto *user = item->getUser();
    const Instruction *InstUser = dynamic_cast<const Instruction *>(user);
    assert(InstUser && "The user of BasicBlock must be Instruction.");
    auto Parent = InstUser->getParent();
    Predecessors.emplace_back(Parent);
  }
  return Predecessors;
}

std::list<std::shared_ptr<Instruction>>::iterator BasicBlock::Insert(Iterator InsertP, std::shared_ptr<Instruction> I) {
  return InstList.insert(InsertP, I);
}

Iterator BasicBlock::getIterator(std::shared_ptr<Instruction> I) {
  for (Iterator begin = InstList.begin(), end = InstList.end(); begin != end;
       begin++) {
    if (*begin == I) {
      return begin;
    }
  }
  return InstList.end();
}

std::list<std::shared_ptr<Instruction>>::iterator BasicBlock::begin() { return InstList.begin(); }

std::list<std::shared_ptr<Instruction>>::iterator BasicBlock::end() { return InstList.end(); }

std::shared_ptr<TerminatorInst> BasicBlock::getTerminator() {
  if (InstList.empty())
    return nullptr;
  return std::dynamic_pointer_cast<TerminatorInst>(InstList.back());
}

// Shit code!
bool BasicBlock::RemoveInst(const Value *val) {
  for (auto iter = InstList.begin(), end = InstList.end(); iter != end;
       iter++) {
    if (val == (*iter).get()) {
      InstList.erase(iter);
      return true;
    }
  }
  return false;
}

/// \brief Remove 'this' from the containing function.
/// \returns the element after the erased one.
std::shared_ptr<BasicBlock> BasicBlock::removeFromParent() {
  auto BlockList = getParent()->getBasicBlockList();
  for (std::list<std::shared_ptr<BasicBlock>>::iterator begin = BlockList.begin(),
                                  end = BlockList.end();
       begin != end; begin++) {
    if ((*begin).get() == this)
      return *(BlockList.erase(begin));
  }
  assert(0 && "Parent function doesn't contain this basic block?");
  return nullptr;
}

void BasicBlock::removePredecessor([[maybe_unused]] std::shared_ptr<BasicBlock> Pred) {}

/// \brief slpitBasicBlock - This splits a basic block into two at the specified
/// instruction.
std::shared_ptr<BasicBlock> BasicBlock::splitBasicBlock([[maybe_unused]] unsigned index,
                                  [[maybe_unused]] std::string BBName) {
  return nullptr;
}

/// \brief Print the BasicBlock info.
/// e.g.	entry:
///				%retval = alloca i32
///				...
///				br i1 %cmp, label %if.then, label %if.end
///
///			if.then:
///				%tmp3 = load i32* %num
void BasicBlock::Print(std::ostringstream &out) {
  out << " " << Name << ":\n";
  for (const auto &item : InstList) {
    item->Print(out);
  }
}