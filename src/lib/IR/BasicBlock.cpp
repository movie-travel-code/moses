//===-----------------------------BasicBlock.cpp---------------------------===//
//
// This file implements the BasicBlock class for the IR library.
//
//===----------------------------------------------------------------------===//
#include "../../include/IR/BasicBlock.h"
#include "../../include/IR/Instruction.h"
#include "../../include/IR/ConstantAndGlobal.h"
#include "../../include/IR/IRType.h"
using namespace compiler::IR;

BasicBlock::BasicBlock(std::string Name, FuncPtr Parent, BBPtr InsertBefore)
: Value(std::make_shared<Type>(Type::TypeID::LabelTy), Value::ValueTy::BasicBlockVal, Name)
{}

BBPtr BasicBlock::Create(std::string Name, FuncPtr Parent, BBPtr InsertBefore)
{
	return std::make_shared<BasicBlock>(Name, Parent, InsertBefore);
}

void BasicBlock::Insert(Iterator InsertP, InstPtr I)
{
	InstList.insert(InsertP, I);
}

Iterator BasicBlock::getIterator(InstPtr I)
{
	for (Iterator begin = InstList.begin(), end = InstList.end();
		begin != end; begin++)
	{
		if (*begin == I)
		{
			return begin;
		}
	}
	return InstList.end();
}

std::list<InstPtr>::iterator BasicBlock::end()
{
	return InstList.end();
}

void BasicBlock::setParent(FuncPtr parent)
{

}

void BasicBlock::setName(std::string Name, SymTabPtr ST)
{}

std::shared_ptr<TerminatorInst> BasicBlock::getTerminator()
{
	if (InstList.empty()) return nullptr;
	return std::dynamic_pointer_cast<TerminatorInst>(InstList.back());
}

void BasicBlock::removePredecessor(BBPtr Pred)
{

}

/// \brief slpitBasicBlock - This splits a basic block into two at the specified
/// instruction.
BBPtr BasicBlock::splitBasicBlock(unsigned index, std::string BBName)
{
	return nullptr;
}