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
Argument::Argument(TyPtr Ty, std::string Name, FuncPtr F)
    : Value(Ty, ValueTy::ArgumentVal, Name), Parent(F) {}

void Argument::Print(std::ostringstream &out) {
  Ty->Print(out);
  out << " " << Name;
}
//===---------------------------------------------------------------------===//
// Implement class function.
Function::Function(FuncTypePtr Ty, std::string Name,
                   std::vector<std::string> Names)
    : GlobalValue(PointerType::get(Ty), Value::ValueTy::FunctionVal, Name),
      ReturnType(Ty->getReturnType()), FunctionTy(Ty) {
  // Create space for argument and set the name later.
  for (unsigned i = 0, size = Ty->getNumParams(); i < size; i++) {
    auto ty = (*Ty)[i + 1];
    Arguments.push_back(std::make_shared<Argument>(ty, Names[i]));
  }
}

/// \brief Get the argument.
ArgPtr Function::operator[](unsigned index) const {
  assert(index < Arguments.size() &&
         "Index out of range when we get the specified Argument(IR).");
  return Arguments[index];
}

/// \brief Set argument info.
void Function::setArgumentInfo(unsigned index, std::string name) {
  assert(index < Arguments.size() &&
         "Index out of range when set Argument(IR) name.");
  Arguments[index]->setName(name);
}

/// \brief Create a new function.
FuncPtr Function::create(FuncTypePtr Ty, std::string Name,
                         std::vector<std::string> Names) {
  auto func = std::make_shared<Function>(Ty, Name, Names);
  return func;
}

TyPtr Function::getReturnType() const {
  if (FuncTypePtr ty = std::dynamic_pointer_cast<FunctionType>(FunctionTy))
    return ty->getReturnType();
  return nullptr;
}

/// \brief Print the Function Info.
/// e.g.	define i32 @add(i32 %parm) {
///				entry:
///					...
///				if.then:
///			}
void Function::Print(std::ostringstream &out) {
  out << "define";
  ReturnType->Print(out);
  out << " " << Name << "(";
  if (!Arguments.empty()) {
    unsigned ArgNum = Arguments.size();
    for (unsigned i = 0; i < ArgNum; i++) {
      Arguments[i]->Print(out);
      if (i < ArgNum - 1) {
        out << ",";
      }
    }
  }
  out << ")\n{\n";
  for (auto item : BasicBlocks) {
    item->Print(out);
  }
  out << "}\n";
}

void Intrinsic::Print(std::ostringstream &out) {}