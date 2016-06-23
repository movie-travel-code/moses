//===--------------------------Instruction.cpp----------------------------===//
// This file implements moses IR instructions.
//===---------------------------------------------------------------------===//
#include "../../include/IR/Instruction.h"

using namespace compiler::IR;
//===---------------------------------------------------------------------===//
// Implements Instruction.
Instruction::Instruction(TyPtr Ty, Opcode op, unsigned NumOps, InstPtr InsertBefore) : 
	User(Ty, Value::ValueTy::InstructionVal)
{

}

Instruction::Instruction(TyPtr Ty, Opcode op, unsigned NumOps, BBPtr InsertAtEnd) : 
	User(Ty, Value::ValueTy::InstructionVal)
{

}

//===---------------------------------------------------------------------===//
// Implements CmpInst.
CmpInst::CmpInst(InstPtr InsertBefore, Predicate pred, ValPtr LHS, ValPtr RHS, std::string NameStr) : 
	Instruction(nullptr, Opcode::Cmp, 2)
{

}

CmpInstPtr CmpInst::Create(Predicate predicate, ValPtr S1, ValPtr S2, std::string name, InstPtr InsertBefore)
{
	return std::make_shared<CmpInst>(nullptr, predicate, S1, S2, name);
}

CmpInstPtr CmpInst::Create(Predicate predicate, ValPtr S1, ValPtr S2,
	std::string name, BBPtr InsertAtEnd)
{
	return std::make_shared<CmpInst>(nullptr, predicate, S1, S2);
}

//===---------------------------------------------------------------------===//
// Implements BinaryOperator Instructions.
BinaryOperator::BinaryOperator(Opcode op, ValPtr S1, ValPtr S2, TyPtr Ty, BBPtr InsertAtEnd) 
	: Instruction(Ty, op, 2, InsertAtEnd)
{
}

BinaryOperator::BinaryOperator(Opcode op, ValPtr S1, ValPtr S2, TyPtr Ty, InstPtr InsertBefore)
	: Instruction(Ty, op, 2, InsertBefore)
{
}

BOInstPtr BinaryOperator::Create(Opcode op, ValPtr S1, ValPtr S2, std::string Name, BBPtr InsertAtEnd)
{
	// SetName()
	return std::make_shared<BinaryOperator>(op, S1, S2, S1->getType(), InsertAtEnd);
}

BOInstPtr BinaryOperator::Create(Opcode op, ValPtr S1, ValPtr S2, std::string Name, InstPtr InsertBefore)
{
	// SetName()
	return std::make_shared<BinaryOperator>(op, S1, S2, S1->getType(), InsertBefore);
}

BOInstPtr BinaryOperator::CreateNeg(ValPtr Operand, std::string Name, InstPtr InsertBefore)
{
	ConstantIntPtr zero = ConstantInt::getZeroValueForNegative();
	return Create(Opcode::Sub, zero, Operand, Name, InsertBefore);
}

BOInstPtr BinaryOperator::CreateNeg(ValPtr Operand, std::string Name, BBPtr InsertAtEnd)
{
	ConstantIntPtr zero = ConstantInt::getZeroValueForNegative();
	return Create(Opcode::Sub, zero, Operand, Name, InsertAtEnd);
}

BOInstPtr BinaryOperator::CreateNot(ValPtr Operand, std::string Name, InstPtr InsertBefore)
{
	ConstantBoolPtr True = ConstantBool::getTrue();
	return Create(Opcode::Xor, Operand, True, Name, InsertBefore);
}

BOInstPtr BinaryOperator::CreateNot(ValPtr Operand, std::string Name, BBPtr InsertAtEnd)
{
	ConstantBoolPtr True = ConstantBool::getTrue();
	return Create(Opcode::Xor, Operand, True, Name, InsertAtEnd);
}

//===---------------------------------------------------------------------===//
// Implements StoreInst.
StoreInst::StoreInst(ValPtr Val, ValPtr Ptr, BBPtr InsertAtEnd) : 
	Instruction(nullptr, Opcode::Store, 2)
{

}

//===---------------------------------------------------------------------===//
// Implements AllocInst.
AllocaInst::AllocaInst(TyPtr Ty, ValPtr Val, std::string Name, InstPtr InsertBefore) :
	UnaryOperator(Ty, Opcode::Alloca, Val, InsertBefore)
{

}

AllocaInst::AllocaInst(TyPtr Ty, BBPtr InsertAtEnd) :
	UnaryOperator(Ty, Opcode::Alloca, nullptr, nullptr)
{

}

AllocaInst::~AllocaInst()
{}

//===---------------------------------------------------------------------===//
// Implements LoadInst.
LoadInst::LoadInst(TyPtr Ty, ValPtr Ptr, std::string Name, InstPtr InsertBefore) :
	UnaryOperator(Ty, Opcode::Load, Ptr, InsertBefore)
{
}

LoadInst::LoadInst(TyPtr Ty, ValPtr Ptr, std::string Name, BBPtr InsertAtEnd) :
	UnaryOperator(Ty, Opcode::Load, Ptr, nullptr)
{

}

LoadInst::~LoadInst() {}

//===---------------------------------------------------------------------===//
// Implements ReturnInst.
ReturnInst::ReturnInst(ValPtr retVal, InstPtr InsertBefore) : 
	TerminatorInst(retVal->getType(), Opcode::Ret, 1)
{

}

ReturnInst::~ReturnInst() {}

ValPtr ReturnInst::getReturnValue() const { return Val; }

BBPtr ReturnInst::getSuccessorV(unsigned index) const
{
	return nullptr;
}
unsigned ReturnInst::getNumSuccessorV() const
{
	return 0;
}
void ReturnInst::setSuccessorV(unsigned idx, BBPtr B)
{

}