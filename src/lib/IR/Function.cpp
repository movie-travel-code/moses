//===----------------------------Function.cpp-----------------------------===//
//
// This file implements the class function.
//
//===---------------------------------------------------------------------===//
#include "../../include/IR/Function.h"
using namespace compiler::IR;

//===---------------------------------------------------------------------===//
// Implement class Argument.
Argument::Argument(TyPtr Ty, std::string Name, FuncPtr F) : 
	Value(Ty, ValueTy::ArgumentVal, Name), Parent(F) {}

//===---------------------------------------------------------------------===//
// Implement class function.
FuncPtr Function::create(FuncTypePtr Ty, std::string Name)
{
	return std::make_shared<Function>(Ty, Name);
}

TyPtr Function::getReturnType() const
{
	if (FuncTypePtr ty = std::dynamic_pointer_cast<FunctionType>(Ty))
		return ty->getReturnType();
}