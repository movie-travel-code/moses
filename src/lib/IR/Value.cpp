//===--------------------------------Value.cpp----------------------------===//
//
// This file implements the Value and user classes.
//
//===---------------------------------------------------------------------===//
#include "../../include/IR/Value.h"
using namespace compiler::IR;
Value::Value(std::shared_ptr<Type> ty, ValueTy vty, std::string name) :
Ty(ty), VTy(vty), Name(name)
{}

/// \brief Change all uses of this to point to a new Value.
///
/// Go through the uses list for this definition and make each use point
/// to 'V' instead of 'this'. After this completes, this's use list is
/// guaranteed to be empty.
void Value::replaceAllUsesWith(ValPtr NewV)
{
	assert(NewV && "Value::replaceAllUsesWith(<null>) is invalid!");
	assert(NewV.get() != this && "this->replaceAllUsesWith(expr(this)) is NOT valid!");
	assert(NewV->getType() == getType() &&
		"replaceAllUses of value with new value of different type!");

	// Replace
	for (auto item : Uses)
	{
		item->set(NewV);
	}
	// Clear the Uses.
	Uses.clear();
}

void Value::addUse(Use& U)
{
	Uses.push_back(&U);
}

void Value::killUse(Use& U)
{
	Uses.remove(&U);
}

Value::~Value()
{
	// Tell the Users, this value is gone,
}

Use::Use(ValPtr Val, User* U) : Val(Val), U(U)
{
	if (Val) Val->addUse(*this);
}
Use::Use(const Use& u) : Val(u.Val), U(u.U)
{
	if (Val) Val->addUse(*this);
}
Use::~Use() { if (Val) Val->killUse(*this); }

void Use::set(ValPtr Val)
{
	if (this->Val) this->Val->killUse(*this);
	this->Val = Val;
	if (Val) Val->addUse(*this);
}
ValPtr Use::operator=(ValPtr RHS)
{
	set(RHS);
	return RHS;
}
const Use& Use::operator=(Use RHS)
{
	set(RHS.Val);
	return *this;
}
bool Use::operator==(const Use& use)
{
	if (Val == use.Val && U == use.U)
		return true;
	return false;
}