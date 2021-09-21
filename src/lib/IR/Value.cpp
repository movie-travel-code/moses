//===--------------------------------Value.cpp----------------------------===//
//
// This file implements the Value and user classes.
//
//===---------------------------------------------------------------------===//
#include "include/IR/Value.h"
using namespace IR;
Value::Value(std::shared_ptr<Type> ty, ValueTy vty, const std::string &name)
    : Name(name), VTy(vty), Ty(ty) {}

bool Value::hasOneUse() const {
  if (Uses.size() == 1)
    return true;
  return false;
}

const Value *Value::use_begin() const { return Uses.front()->getUser(); }

/// \brief Change all uses of this to point to a new Value.
///
/// Go through the uses list for this definition and make each use point
/// to 'V' instead of 'this'. After this completes, this's use list is
/// guaranteed to be empty.
void Value::replaceAllUsesWith(ValPtr NewV) {
  assert(NewV && "Value::replaceAllUsesWith(<null>) is invalid!");
  assert(NewV.get() != this &&
         "this->replaceAllUsesWith(expr(this)) is NOT valid!");
  assert(NewV->getType() == getType() &&
         "replaceAllUses of value with new value of different type!");

  // Replace
  for (auto item : Uses) {
    item->set(NewV);
  }
  // Clear the Uses.
  Uses.clear();
}

void Value::addUse(Use &U) { Uses.push_back(&U); }

void Value::killUse(Use &U) { Uses.remove(&U); }

Value::~Value() {
  // Tell the Users, this value is gone,
}

Use::Use(ValPtr Val, User *U) : U(U), Val(Val) {
  if (Val)
    Val->addUse(*this);
}
Use::Use(const Use &u) : U(u.U), Val(u.Val) {
  if (Val)
    Val->addUse(*this);
}
Use::~Use() {
  if (Val)
    Val->killUse(*this);
}

void Use::set(ValPtr Val) {
  if (this->Val)
    this->Val->killUse(*this);
  this->Val = Val;
  if (Val)
    Val->addUse(*this);
}
ValPtr Use::operator=(ValPtr RHS) {
  set(RHS);
  return RHS;
}
const Use &Use::operator=(Use RHS) {
  set(RHS.Val);
  return *this;
}
bool Use::operator==(const Use &use) {
  if (Val == use.Val && U == use.U)
    return true;
  return false;
}