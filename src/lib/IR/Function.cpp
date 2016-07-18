//===----------------------------Function.cpp-----------------------------===//
//
// This file implements the class function.
//
//===---------------------------------------------------------------------===//
#include "../../include/IR/Function.h"
#include "../../include/IR/BasicBlock.h"
using namespace compiler::IR;

//===---------------------------------------------------------------------===//
// Implement class Argument.
Argument::Argument(TyPtr Ty, std::string Name, FuncPtr F) : 
	Value(Ty, ValueTy::ArgumentVal, Name), Parent(F) {}

void Argument::Print(std::ostringstream& out)
{
	Ty->Print(out);
	out << " " << Name;
}
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

/// \brief Print the Function Info.
/// e.g.	define i32 @add(i32 %parm) {
///				entry:
///					...
///				if.then:
///			}
void Function::Print(std::ostringstream& out)
{
	out << "define";
	ReturnType->Print(out);
	out << " " << Name << "(";
	if (!Arguments.empty())
	{
		unsigned ArgNum = Arguments.size();
		for (unsigned i = 0; i < ArgNum; i++)
		{
			Arguments[i]->Print(out);
			if (i == ArgNum - 1) { out << ","; }
		}
	}
	out << ")\n{\n";
	for (auto item : BasicBlocks)
	{
		item->Print(out);
	}
	out << "}\n";
}