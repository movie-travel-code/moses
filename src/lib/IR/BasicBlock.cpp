//===-----------------------------BasicBlock.cpp---------------------------===//
//
// This file implements the BasicBlock class for the IR library.
//
//===----------------------------------------------------------------------===//
#include "../../include/IR/BasicBlock.h"
#include "../../include/IR/ConstantAndGlobal.h"
#include "../../include/IR/IRType.h"
#include "../../include/IR/Instruction.h"

using namespace compiler::IR;

BasicBlock::BasicBlock(std::string Name, FuncPtr Parent, BBPtr InsertBefore)
    : Value(std::make_shared<Type>(Type::TypeID::LabelTy),
            Value::ValueTy::BasicBlockVal, Name),
      Parent(Parent) {}

BBPtr BasicBlock::Create(std::string Name, FuncPtr Parent, BBPtr InsertBefore) {
  return std::make_shared<BasicBlock>(Name, Parent, InsertBefore);
}

// Get the predecessors of this basic block.
std::vector<BBPtr> BasicBlock::getPredecessors() const {
  std::vector<BBPtr> Predecessors;
  // (1) get the use list.
  auto Uses = getUses();

  // (2) get the use's father
  for (auto item : Uses) {
    auto user = item->getUser();
    const Instruction *InstUser = dynamic_cast<const Instruction *>(user);
    assert(InstUser && "The user of BasicBlock must be Instruction.");
    auto Parent = InstUser->getParent();
    Predecessors.push_back(Parent);
  }
  return Predecessors;
}

std::list<InstPtr>::iterator BasicBlock::Insert(Iterator InsertP, InstPtr I) {
  return InstList.insert(InsertP, I);
}

Iterator BasicBlock::getIterator(InstPtr I) {
  for (Iterator begin = InstList.begin(), end = InstList.end(); begin != end;
       begin++) {
    if (*begin == I) {
      return begin;
    }
  }
  return InstList.end();
}

std::list<InstPtr>::iterator BasicBlock::begin() { return InstList.begin(); }

std::list<InstPtr>::iterator BasicBlock::end() { return InstList.end(); }

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
BBPtr BasicBlock::removeFromParent() {
  auto BlockList = getParent()->getBasicBlockList();
  for (std::list<BBPtr>::iterator begin = BlockList.begin(),
                                  end = BlockList.end();
       begin != end; begin++) {
    if ((*begin).get() == this)
      return *(BlockList.erase(begin));
  }
  assert(0 && "Parent function doesn't contain this basic block?");
  return nullptr;
}

void BasicBlock::removePredecessor(BBPtr Pred) {}

/// \brief slpitBasicBlock - This splits a basic block into two at the specified
/// instruction.
BBPtr BasicBlock::splitBasicBlock(unsigned index, std::string BBName) {
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
  for (auto item : InstList) {
    item->Print(out);
  }
}