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
: Value(std::make_shared<Type>(Type::TypeID::LabelTyID), Value::ValueTy::BasicBlockVal, Name)
{}

BBPtr BasicBlock::Create(std::string Name, FuncPtr Parent, BBPtr InsertBefore)
{
	return std::make_shared<BasicBlock>(Name, Parent, InsertBefore);
}

void BasicBlock::setParent(FuncPtr parent)
{

}

void BasicBlock::setName(std::string Name, SymTabPtr ST)
{}

InstPtr BasicBlock::getTerminator()
{
	return InstList.back();
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