//===--------------------------ConstAndGlobal.cpp-------------------------===//
//
// Impletation for class 'const' and class 'global'.
//
//===---------------------------------------------------------------------===//
#include "include/IR/ConstantAndGlobal.h"
#include "include/IR/MosesIRContext.h"

using namespace IR;
//===-----------------------------------------------------------===//
// Implements the GlobalValue.
GlobalValue::GlobalValue(TyPtr Ty, ValueTy vty, const std::string &name)
    : User(PointerType::get(Ty), vty, name), ValTy(Ty) {}

//===-----------------------------------------------------------===//
// Implements the ConstantBool.
ConstantBool::ConstantBool(MosesIRContext &Ctx, bool val)
    : ConstantIntegral(Ctx.getBoolTy()), Val(val) {}

void ConstantBool::Print(std::ostringstream &out) {
  if (Val)
    out << " true";
  else
    out << " false";
}

//===-----------------------------------------------------------===//
// Implements the ConstantInt.
ConstantInt::ConstantInt(MosesIRContext &Ctx, int val)
    : ConstantIntegral(Ctx.getIntTy()), Val(val) {}

void ConstantInt::Print(std::ostringstream &out) {
  out << " " << std::to_string(Val);
}
