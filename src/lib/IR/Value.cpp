//===--------------------------------Value.cpp----------------------------===//
//
// This file implements the Value and user classes.
//
//===---------------------------------------------------------------------===//
#include "../../include/IR/Value.h"

namespace compiler
{
	namespace IR
	{
		Value::Value(std::shared_ptr<Type> ty, ValueTy vty, std::string name) :
			Ty(ty), VTy(vty), Name(name)
		{}

		Value::~Value()
		{
			// Notify all ValueHandles (if present) that this value is going away.
		}
	}
}