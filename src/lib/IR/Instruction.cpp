//===--------------------------Instruction.cpp----------------------------===//
// This file implements moses IR instructions.
//===---------------------------------------------------------------------===//
#include "IR/Instruction.h"
#include "IR/IRType.h"
#include "IR/MosesIRContext.h"

using namespace IR;
//===---------------------------------------------------------------------===//
// Implements Instruction.
Instruction::Instruction(TyPtr Ty, Opcode op, std::shared_ptr<BasicBlock> parent,
                         const std::string &Name)
    : User(Ty, Value::ValueTy::InstructionVal, Name), Parent(parent), op(op) {}

std::shared_ptr<Function> Instruction::getFunction() const { return Parent->getParent(); }

bool Instruction::mayReadFromMemory() const {
  if (op == Opcode::Load)
    return true;
  return false;
}

bool Instruction::mayWriteToMemory() const {
  if (op == Opcode::Store)
    return true;
  return false;
}
// Instruction::Instruction(TyPtr Ty, Opcode op, unsigned NumOps, std::shared_ptr<BasicBlock>
// InsertAtEnd) : 	User(Ty, Value::ValueTy::InstructionVal)
//{
//
//}

//===---------------------------------------------------------------------===//
// Implements CmpInst.
CmpInst::CmpInst(MosesIRContext &Ctx, [[maybe_unused]] std::shared_ptr<Instruction> InsertBefore,
                 Predicate pred, std::shared_ptr<Value> LHS, std::shared_ptr<Value> RHS, std::shared_ptr<BasicBlock> parent,
                 const std::string &Name)
    : Instruction(Type::getBoolType(Ctx), Opcode::Cmp, parent, Name),
      predicate(pred) {
  // Operands.resize(2);
  Operands.push_back(Use(LHS, this));
  Operands.push_back(Use(RHS, this));
  // To Do: some checks.
}

CmpInst::~CmpInst() {}

std::shared_ptr<CmpInst> CmpInst::Create(MosesIRContext &Ctx, Predicate predicate, std::shared_ptr<Value> S1,
                           std::shared_ptr<Value> S2, std::shared_ptr<BasicBlock> parent, const std::string &name,
                           [[maybe_unused]] std::shared_ptr<Instruction> InsertBefore) {
  return std::make_shared<CmpInst>(Ctx, nullptr, predicate, S1, S2, parent,
                                   name);
}

std::shared_ptr<CmpInst> CmpInst::Create(MosesIRContext &Ctx, Predicate predicate, std::shared_ptr<Value> S1,
                           std::shared_ptr<Value> S2, std::shared_ptr<BasicBlock> parent, const std::string &name,
                           [[maybe_unused]] std::shared_ptr<BasicBlock> InsertAtEnd) {
  return std::make_shared<CmpInst>(Ctx, nullptr, predicate, S1, S2, parent,
                                   name);
}

/// \brief Print CmpInst.
/// e.g.	if (parm > 0)
///			%tmp = load i32* %parm.addr
///			%cmp = cmp gt i32 %parm, 0			; <i1>
void CmpInst::Print(std::ostringstream &out) {
  std::string PreStr;
  switch (predicate) {
  case Predicate::CMP_EQ:
    PreStr = " eq";
    break;
  case Predicate::CMP_GE:
    PreStr = " ge";
    break;
  case Predicate::CMP_GT:
    PreStr = " gt";
    break;
  case Predicate::CMP_LE:
    PreStr = " le";
    break;
  case Predicate::CMP_LT:
    PreStr = " lt";
    break;
  case Predicate::CMP_NE:
    PreStr = " ne";
    break;
  default:
    assert(0 && "Unreachable code!");
    break;
  }
  out << Name << " = cmp";
  out << PreStr;
  Operands[0].get()->getType()->Print(out);
  out << " " << Operands[0].get()->getName() << ",";
  Operands[1].get()->getType()->Print(out);
  out << " " << Operands[1].get()->getName();
  out << "        ; < ";
  Ty->Print(out);
  out << " > \n";
}

//===---------------------------------------------------------------------===//
// Implements BinaryOperator Instructions.
BinaryOperator::BinaryOperator(Opcode op, std::shared_ptr<Value> S1, std::shared_ptr<Value> S2, TyPtr Ty,
                               std::shared_ptr<BasicBlock> parent,
                               [[maybe_unused]] const std::string &Name)
    : Instruction(Ty, op, parent) {
  // Operands.resize(2);
  Operands.push_back(Use(S1, this));
  Operands.push_back(Use(S2, this));
  assert(S1 && S2 && S1->getType() == S2->getType());
}

BinaryOperator::~BinaryOperator() {}

std::shared_ptr<BinaryOperator> BinaryOperator::Create(Opcode op, std::shared_ptr<Value> S1, std::shared_ptr<Value> S2, std::shared_ptr<BasicBlock> parent,
                                 [[maybe_unused]] std::shared_ptr<BasicBlock> InsertAtEnd) {
  assert(S1->getType() == S2->getType() &&
         "Cannot create binary operator with two operands of differing type!");
  return std::make_shared<BinaryOperator>(op, S1, S2, S1->getType(), parent);
}

std::shared_ptr<BinaryOperator> BinaryOperator::Create(Opcode op, std::shared_ptr<Value> S1, std::shared_ptr<Value> S2, std::shared_ptr<BasicBlock> parent,
                                 [[maybe_unused]] std::shared_ptr<Instruction> InsertBefore) {
  return std::make_shared<BinaryOperator>(op, S1, S2, S1->getType(), parent);
}

std::shared_ptr<BinaryOperator> BinaryOperator::CreateNeg(MosesIRContext &Ctx, std::shared_ptr<Value> Operand,
                                    std::shared_ptr<BasicBlock> parent, std::shared_ptr<Instruction> InsertBefore) {
  std::shared_ptr<ConstantInt> zero = ConstantInt::getZeroValueForNegative(Ctx);
  return Create(Opcode::Sub, zero, Operand, parent, InsertBefore);
}

std::shared_ptr<BinaryOperator> BinaryOperator::CreateNeg(MosesIRContext &Ctx, std::shared_ptr<Value> Operand,
                                    std::shared_ptr<BasicBlock> parent, std::shared_ptr<BasicBlock> InsertAtEnd) {
  std::shared_ptr<ConstantInt> zero = ConstantInt::getZeroValueForNegative(Ctx);
  return Create(Opcode::Sub, zero, Operand, parent, InsertAtEnd);
}

std::shared_ptr<BinaryOperator> BinaryOperator::CreateNot(MosesIRContext &Ctx, std::shared_ptr<Value> Operand,
                                    std::shared_ptr<BasicBlock> parent, std::shared_ptr<Instruction> InsertBefore) {
  std::shared_ptr<ConstantBool> True = ConstantBool::getTrue(Ctx);
  return Create(Opcode::Xor, Operand, True, parent, InsertBefore);
}

/// Shit code!
std::shared_ptr<BinaryOperator> BinaryOperator::CreateNot(MosesIRContext &Ctx, std::shared_ptr<Value> Operand,
                                    std::shared_ptr<BasicBlock> parent, std::shared_ptr<BasicBlock> InsertAtEnd) {
  std::shared_ptr<ConstantBool> True = ConstantBool::getTrue(Ctx);
  return Create(Opcode::Xor, Operand, True, parent, InsertAtEnd);
}

/// \brief Print the BinaryOperator info.
/// e.g.	parm - 1
///			%sub = sub i32 %tmp1, 1
void BinaryOperator::Print(std::ostringstream &out) {
  out << Name << " =";
  switch (op) {
  case IR::Instruction::Opcode::Add:
    out << " add";
    break;
  case IR::Instruction::Opcode::Sub:
    out << " sub";
    break;
  case IR::Instruction::Opcode::Mul:
    out << " mul";
    break;
  case IR::Instruction::Opcode::Div:
    out << " div";
    break;
  case IR::Instruction::Opcode::Rem:
    out << " rem";
    break;
  case IR::Instruction::Opcode::Shl:
    out << " shl";
    break;
  case IR::Instruction::Opcode::Shr:
    out << " shr";
    break;
  case IR::Instruction::Opcode::And:
    out << " and";
    break;
  case IR::Instruction::Opcode::Or:
    out << " or";
    break;
  case IR::Instruction::Opcode::Xor:
    out << " xor";
    break;
  default:
    break;
  }
  Operands[0].get()->getType()->Print(out);
  out << " " << Operands[0].get()->getName() << ",";
  Operands[0].get()->getType()->Print(out);
  out << " " << Operands[1].get()->getName();
  out << "        ; <";
  Ty->Print(out);
  out << " >\n";
}
//===---------------------------------------------------------------------===//
// StoreInst Implementation
StoreInst::StoreInst(MosesIRContext &Ctx, std::shared_ptr<Value> Val, std::shared_ptr<Value> Ptr, std::shared_ptr<BasicBlock> parent,
                     [[maybe_unused]] std::shared_ptr<BasicBlock> InsertAtEnd)
    : Instruction(Type::getVoidType(Ctx), Opcode::Store, parent) {
  // Operands.resize(2);
  Operands.push_back(Use(Val, this));
  Operands.push_back(Use(Ptr, this));
  assert(Val && Ptr && "Both operands must be non-null!");
  // e.g. %retval = alloca i32
  //      store i32 10, i32* %retval
  // %retval must have i32* type.
  assert(Ptr->getType()->isPointerTy() && "Ptr must have pointer type!");

  auto dynptrty = std::dynamic_pointer_cast<PointerType>(Ptr->getType());

  assert(Val->getType().get() ==
             std::dynamic_pointer_cast<PointerType>(Ptr->getType())
                 ->getElementTy()
                 .get() &&
         "Ptr must be a pointer to Val type!");
}

std::shared_ptr<StoreInst> StoreInst::Create(MosesIRContext &Ctx, std::shared_ptr<Value> Val, std::shared_ptr<Value> Ptr,
                               std::shared_ptr<BasicBlock> parent) {
  return std::make_shared<StoreInst>(Ctx, Val, Ptr, parent);
}

/// \brief Print the StoreInst info.
/// e.g.	return parm - 1;
///			%add = add i32 %tmp, 1
///			store i32 %add, i32* %retval
void StoreInst::Print(std::ostringstream &out) {
  out << "store";
  Operands[0].get()->getType()->Print(out);
  out << " " << Operands[0].get()->getName() << ",";
  Operands[1].get()->getType()->Print(out);
  out << " " << Operands[1].get()->getName();
  out << "        ; <";
  Ty->Print(out);
  out << " >\n";
}

//===---------------------------------------------------------------------===//
// Implements AllocInst.
AllocaInst::AllocaInst(TyPtr Ty, std::shared_ptr<BasicBlock> parent,
                       [[maybe_unused]] const std::string &Name,
                       [[maybe_unused]] std::shared_ptr<BasicBlock> InsertAtEnd)
    : UnaryOperator(PointerType::get(Ty), Opcode::Alloca, nullptr, parent),
      AllocatedType(Ty) {}

AllocaInst::~AllocaInst() {}

std::shared_ptr<AllocaInst>  AllocaInst::Create(TyPtr Ty, std::shared_ptr<BasicBlock> parent,
                                 const std::string &Name) {
  return std::make_shared<AllocaInst>(Ty, parent, Name);
}

/// \brief Print the AllocaInst info.
/// e.g.	func add() -> int
///
///			%retval = alloca i32
void AllocaInst::Print(std::ostringstream &out) {
  out << Name << " = alloca";
  AllocatedType->Print(out);
  out << "        ; <";
  Ty->Print(out);
  out << " >\n";
}

//===-------------------------------------------------------------===//
//						GetElementPtrInst Class
void GetElementPtrInst::init(std::shared_ptr<Value> Ptr, std::vector<std::shared_ptr<Value>> Idx) {
  Operands.push_back(Use(Ptr, this));
  for (std::size_t i = 0, size = Idx.size(); i < size; i++)
    Operands.push_back(Use(Idx[i], this));
}

void GetElementPtrInst::init(std::shared_ptr<Value> Ptr, std::shared_ptr<Value> Idx0, std::shared_ptr<Value> Idx1) {
  Operands.push_back(Use(Ptr, this));
  Operands.push_back(Use(Idx0, this));
  Operands.push_back(Use(Idx1, this));
}

GetElementPtrInst::GetElementPtrInst(TyPtr PointeeType, std::shared_ptr<Value> Ptr,
                                     std::vector<std::shared_ptr<Value>> IdxList, std::shared_ptr<BasicBlock> parent,
                                     [[maybe_unused]] const std::string &Name,
                                     [[maybe_unused]] std::shared_ptr<Instruction> InsertBefore)
    : Instruction(PointerType::get(PointeeType), Opcode::GetElementPtr,
                  parent) {
  init(Ptr, IdxList);
}

GetElementPtrInst::GetElementPtrInst(TyPtr PointeeType, std::shared_ptr<Value> Ptr, std::shared_ptr<Value> Idx0,
                                     std::shared_ptr<Value> Idx1, std::shared_ptr<BasicBlock> parent,
                                     [[maybe_unused]] const std::string &Name)
    : Instruction(PointerType::get(PointeeType), Opcode::GetElementPtr,
                  parent) {
  init(Ptr, Idx0, Idx1);
}

std::shared_ptr<GetElementPtrInst> GetElementPtrInst::Create(TyPtr PointeeType, std::shared_ptr<Value> Ptr, std::shared_ptr<Value> Idx0,
                                     std::shared_ptr<Value> Idx1, std::shared_ptr<BasicBlock> parent,
                                     const std::string &Name,
                                     [[maybe_unused]] std::shared_ptr<Instruction> InsertBefore) {
  return std::make_shared<GetElementPtrInst>(PointeeType, Ptr, Idx0, Idx1,
                                             parent, Name);
}

std::shared_ptr<GetElementPtrInst> GetElementPtrInst::Create(TyPtr PointeeType, std::shared_ptr<Value> Ptr,
                                     std::vector<std::shared_ptr<Value>> IdxList, std::shared_ptr<BasicBlock> parent,
                                     const std::string &Name,
                                     [[maybe_unused]] std::shared_ptr<Instruction> InsertBefore) {
  return std::make_shared<GetElementPtrInst>(PointeeType, Ptr, IdxList, parent,
                                             Name);
}

void GetElementPtrInst::Print(std::ostringstream &out) {
  out << Name << " = getelementptr";
  Operands[0].get()->getType()->Print(out);
  out << " " << Operands[0].get()->getName() << ", ";
  out << "int 0, "
      << "int " << Operands[2].get()->getName() << "\n";
}

//===---------------------------------------------------------------------===//
// Implements the Terminatror inst.
TerminatorInst::~TerminatorInst() {}

//===---------------------------------------------------------------------===//
// Implements CallInst.
CallInst::CallInst(std::shared_ptr<FunctionType> FTy, std::shared_ptr<Value> Func, std::vector<std::shared_ptr<Value>> Args,
                   std::shared_ptr<BasicBlock> parent, [[maybe_unused]] const std::string &Name,
                   [[maybe_unused]] std::shared_ptr<BasicBlock> InsertAtEnd)
    : Instruction(FTy->getReturnType(), Opcode::Call, parent), FTy(FTy),
      IsIntrisicCall(false) {
  // Operands.resize(1 + Args.size());
  Operands.push_back(Use(Func, this));
  assert(Args.size() == FTy->getNumParams() &&
         "The number of parameters is different!");
  std::size_t ArgsNum = Args.size();
  for (unsigned i = 0; i < ArgsNum; i++)
    Operands.push_back(Use(Args[i], this));
}

CallInst::CallInst(std::shared_ptr<Intrinsic> Intr, std::vector<std::shared_ptr<Value>> Args, std::shared_ptr<BasicBlock> parent,
                   [[maybe_unused]] const std::string &Name,
                   [[maybe_unused]] std::shared_ptr<Instruction> InsertBefore)
    : Instruction(nullptr, Opcode::Call, parent), IsIntrisicCall(true) {
  Operands.push_back(Use(Intr, this));
  for (std::size_t i = 0, size = Args.size(); i < size; i++)
    Operands.push_back(Use(Args[i], this));
}

CallInst::~CallInst() {}

std::shared_ptr<CallInst> CallInst::Create(std::shared_ptr<Value> Func, std::vector<std::shared_ptr<Value>> Args,
                             std::shared_ptr<BasicBlock> parent, const std::string &Name,
                             [[maybe_unused]] std::shared_ptr<Instruction> InsertBefore) {
  auto func = std::dynamic_pointer_cast<Function>(Func);
  assert(func && "The Value must be the 'Function'!");
  auto functy =
      std::dynamic_pointer_cast<FunctionType>(func->getFunctionType());
  assert(functy && "The function must have 'FunctionType'");
  return std::make_shared<CallInst>(functy, Func, Args, parent, Name);
}

std::shared_ptr<CallInst> CallInst::Create(std::shared_ptr<Intrinsic> Intr, std::vector<std::shared_ptr<Value>> Args,
                             std::shared_ptr<BasicBlock> parent, const std::string &Name,
                             [[maybe_unused]] std::shared_ptr<Instruction> InsertBefore) {
  return std::make_shared<CallInst>(Intr, Args, parent, Name);
}
std::shared_ptr<Value> CallInst::getArgOperand(std::size_t i) const {
  assert(i < Operands.size() - 1 && "Index out of range!");
  return Operands[i + 1].get();
}

void CallInst::setArgOperand(unsigned i, std::shared_ptr<Value> v) {
  assert(i < Operands.size() && "Index out of range!");
  Operands[i + 1] = v;
}

/// \brief Print the CallInst info.
/// e.g.	int num = 10;
///			add(num);
///
///			%tmp = load i32* %num
///			%call = call i32 @add(i32 %tmp)
///				or
///			call void @add(i32 %tmp)
void CallInst::Print(std::ostringstream &out) {
  // Shit!
  if (!Ty) {
    out << "call";
    out << " " << Operands[0].get()->getName() << "(";
    if (Operands.size() > 1) {
      // Print the parameter information.
      std::size_t Length = Operands.size();
      for (unsigned i = 1; i < Length; i++) {
        Operands[i].get()->getType()->Print(out);
        out << " " << Operands[i].get()->getName();
        if (i != Length - 1) {
          out << ",";
        }
      }
    }
    out << ")"
        << "        ; \n";
    return;
  }

  if (!Ty->isVoidType()) {
    out << Name << " =";
  }
  out << " call";
  Ty->Print(out);
  out << " " << Operands[0].get()->getName() << "(";
  if (Operands.size() > 1) {
    // Print the parameter information.
    std::size_t Length = Operands.size();
    for (unsigned i = 1; i < Length; i++) {
      Operands[i].get()->getType()->Print(out);
      out << " " << Operands[i].get()->getName();
      if (i != Length - 1) {
        out << ",";
      }
    }
  }
  out << ")"
      << "        ; <";
  Ty->Print(out);
  out << ">\n";
}
//===---------------------------------------------------------------------===//
// Implements ReturnInst.
ReturnInst::ReturnInst(std::shared_ptr<BasicBlock> parent, std::shared_ptr<Value> retVal,
                       [[maybe_unused]] std::shared_ptr<BasicBlock> InsertAtEnd)
    : TerminatorInst(retVal->getType(), Opcode::Ret, parent) {
  // Operands.resize(1);
  if (retVal) {
    Operands.push_back(Use(retVal, this));
  }
}

std::shared_ptr<ReturnInst> ReturnInst::Create(std::shared_ptr<BasicBlock> parent, std::shared_ptr<Value> retVal,
                                 std::shared_ptr<BasicBlock> InsertAtEnd) {
  return std::make_shared<ReturnInst>(parent, retVal, InsertAtEnd);
}

ReturnInst::~ReturnInst() {}

// Out-of-line ReturnInst method, put here so the c++ compiler can
// choose to emit the vtable for the class in this translation unit :).
void ReturnInst::setSuccessor([[maybe_unused]] unsigned idx,
                              [[maybe_unused]] std::shared_ptr<BasicBlock> B) {
  assert(0 && "has no successors!");
}

/// \brief Print the ReturnInst info.
///	e.g.	return num;
///			ret i32 %0;
///				or
///			return;
///			ret void;
void ReturnInst::Print(std::ostringstream &out) {
  out << "ret";
  if (Operands.size() == 1) {
    Operands[0].get()->getType()->Print(out);
    std::string name = Operands[0].get()->getName();
    out << " " << name << "\n";
  } else {
    out << " void\n";
  }
}
//===---------------------------------------------------------------------===//
// Implements the BranchInst class.
BranchInst::BranchInst(MosesIRContext &Ctx, std::shared_ptr<BasicBlock> IfTrue, std::shared_ptr<BasicBlock> parent,
                       [[maybe_unused]] std::shared_ptr<BasicBlock> InsertAtEnd)
    : TerminatorInst(Type::getVoidType(Ctx), Opcode::Br, parent) {
  assert(IfTrue && "Dest basic block may not be null!");
  // Operands.resize(1);
  Operands.push_back(Use(IfTrue, this));
}

BranchInst::BranchInst(MosesIRContext &Ctx, std::shared_ptr<BasicBlock> IfTrue, std::shared_ptr<BasicBlock> IfFalse,
                       std::shared_ptr<Value> CondV, std::shared_ptr<BasicBlock> parent,
                       [[maybe_unused]] std::shared_ptr<BasicBlock> InsertAtEnd)
    : TerminatorInst(Type::getVoidType(Ctx), Opcode::Br, parent) {
  assert(IfTrue && IfFalse && "Dest basic blocks may not be null!");
  assert(CondV->getType()->isBoolTy() &&
         "May only branch on boolean predicates!");
  // Operands.resize(3);
  Operands.push_back(Use(IfTrue, this));
  Operands.push_back(Use(IfFalse, this));
  Operands.push_back(Use(CondV, this));
}

BranchInst::~BranchInst() {}

std::shared_ptr<BranchInst> BranchInst::Create(MosesIRContext &Ctx, std::shared_ptr<BasicBlock> IfTrue, std::shared_ptr<BasicBlock> parent,
                             std::shared_ptr<BasicBlock> InsertAtEnd) {
  return std::make_shared<BranchInst>(Ctx, IfTrue, parent, InsertAtEnd);
}

std::shared_ptr<BranchInst> BranchInst::Create(MosesIRContext &Ctx, std::shared_ptr<BasicBlock> IfTrue, std::shared_ptr<BasicBlock> IfFalse,
                             std::shared_ptr<Value> CondV, std::shared_ptr<BasicBlock> parent, std::shared_ptr<BasicBlock> InsertAtEnd) {
  return std::make_shared<BranchInst>(Ctx, IfTrue, IfFalse, CondV, parent,
                                      InsertAtEnd);
}

std::shared_ptr<BasicBlock> BranchInst::getSuccessor(unsigned i) const {
  assert((Operands.size() == 1 && i == 0) ||
         (Operands.size() == 3 && (i == 0 || i == 1)) &&
             "Get successors error!");
  auto Succ = Operands[i].get();
  auto SuccBB = std::dynamic_pointer_cast<BasicBlock>(Succ);
  assert(SuccBB && "Successor must be BasicBlock.");
  return SuccBB;
}

void BranchInst::setSuccessor(unsigned idx, std::shared_ptr<BasicBlock> NewSucc) {
  assert((idx == 0 || idx == 1) && "Index out of range!");
  Operands[idx] = NewSucc;
}

/// \brief Print the BranchInst info.
/// e.g.	if (num > parm)
///
///			br i1 %cmp, label %if.then, label %if.end
void BranchInst::Print(std::ostringstream &out) {
  if (Operands.size() == 3) {
    out << "br bool " << Operands[2].get()->getName() << ", label "
        << Operands[0].get()->getName() << ", label "
        << Operands[1].get()->getName() << "\n";
  } else {
    out << "br label " << Operands[0].get()->getName() << "\n";
  }
}
//===---------------------------------------------------------------------===//
// Implements the Load instruciton.
LoadInst::LoadInst(std::shared_ptr<Value> Addr, std::shared_ptr<BasicBlock> parent,
                   [[maybe_unused]] const std::string &Name,
                   [[maybe_unused]] std::shared_ptr<BasicBlock> InsertAtEnd)
    : UnaryOperator(nullptr, Opcode::Load, Addr, parent) {
  auto PointerTy = std::dynamic_pointer_cast<PointerType>(Addr->getType());
  assert(PointerTy && "Addr must have pointer type.");
  Ty = PointerTy->getElementTy();
}

LoadInst::~LoadInst() {}

std::shared_ptr<LoadInst> LoadInst::Create(std::shared_ptr<Value> Addr, std::shared_ptr<BasicBlock> parent) {
  return std::make_shared<LoadInst>(Addr, parent);
}

/// \brief Print the LoadInst info.
///	e.g.	int num = parm + 10��
///
///			%tmp = load i32* %parm.addr
void LoadInst::Print(std::ostringstream &out) {
  out << Name << " = load";
  Operands[0].get()->getType()->Print(out);
  out << " " << Operands[0].get()->getName();
  out << "        ; <";
  Ty->Print(out);
  out << " >\n";
}
