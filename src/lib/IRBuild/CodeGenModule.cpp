//===--------------------------CodeGenMo
#include "IRBuild/IRBuilder.h"
using namespace IRBuild;
using namespace IR;

//===---------------------------------------------------------------------===//
// Implements class ModuleBuilder.
std::string ModuleBuilder::LocalInstNamePrefix = "%";

ModuleBuilder::ModuleBuilder(std::shared_ptr<Scope> SymbolInfo,
                             MosesIRContext &context)
    : SymbolTree(SymbolInfo), CurScope(SymbolInfo), Context(context),
      Types(CodeGenTypes(context)), CurBB(CreateBasicBlock("entry", nullptr)),
      CurFunc(std::make_shared<FunctionBuilderStatus>()),
      isAllocaInsertPointSetByNormalInsert(false), TempCounter(0) {
  SetInsertPoint(CurBB);
  EntryBlock = CurBB;
  AllocaInsertPoint = InsertPoint;

  IRs.push_back(CurBB);
}

void ModuleBuilder::VisitChildren(
    std::vector<std::shared_ptr<StatementAST>> AST) {
  std::size_t ASTSize = AST.size();
  for (unsigned i = 0; i < ASTSize; i++)
    AST[i].get()->Accept(this);
}

//===---------------------------------------------------------------------===//
// Helper for CodeGen.

//===---------------------------------------------------------------------===//
// private helper method.

void ModuleBuilder::SetInsertPoint(std::shared_ptr<BasicBlock> TheBB) {
  CurBB = TheBB;
  InsertPoint = CurBB->end();
}

void ModuleBuilder::SetInsertPoint(std::shared_ptr<Instruction> I) {
  CurBB = I->getParent();
  InsertPoint = CurBB->getIterator(I);
}

//===---------------------------------------------------------------------===//
// Instruction creation methods: Terminators.
std::shared_ptr<ReturnInst> ModuleBuilder::CreateRetVoid() {
  return InsertHelper(ReturnInst::Create(CurBB));
}

std::shared_ptr<ReturnInst> ModuleBuilder::CreateRet(std::shared_ptr<Value> V) {
  return InsertHelper(ReturnInst::Create(CurBB, V));
}

std::shared_ptr<ReturnInst> ModuleBuilder::CreateAggregateRet([[maybe_unused]] std::vector<std::shared_ptr<Value>> retVals,
                                                [[maybe_unused]] unsigned N) {
  return nullptr;
}

std::shared_ptr<BranchInst> ModuleBuilder::Create(std::shared_ptr<BasicBlock> Dest) {
  return InsertHelper(BranchInst::Create(Context, CurBB, Dest), "");
}

std::shared_ptr<BranchInst> ModuleBuilder::CreateCondBr(std::shared_ptr<Value> Cond, std::shared_ptr<BasicBlock> True, std::shared_ptr<BasicBlock> False) {
  return InsertHelper(BranchInst::Create(Context, True, False, Cond, CurBB),
                      "");
}

std::shared_ptr<BranchInst> ModuleBuilder::CreateBr(std::shared_ptr<BasicBlock> Dest) {
  return InsertHelper(BranchInst::Create(Context, Dest, CurBB), "");
}

//===-------------------------------------------------------------===//
// Instruction creation methods: Binary Operators
std::shared_ptr<BinaryOperator> ModuleBuilder::CreateInsertBinOp(BinaryOperator::Opcode Opc,
                                           std::shared_ptr<Value> LHS, std::shared_ptr<Value> RHS,
                                           std::string Name) {
  std::shared_ptr<BinaryOperator> BO =
      InsertHelper(BinaryOperator::Create(Opc, LHS, RHS, CurBB), Name);
  return BO;
}

std::shared_ptr<Value> ModuleBuilder::CreateAdd(std::shared_ptr<Value> LHS, std::shared_ptr<Value> RHS, std::string Name) {
  if (std::shared_ptr<ConstantInt> LC = std::dynamic_pointer_cast<ConstantInt>(LHS)) {
    if (std::shared_ptr<ConstantInt> RC = std::dynamic_pointer_cast<ConstantInt>(RHS))
      return InsertHelper(
          ConstantFolder::CreateArithmetic(Context, Opcode::Add, LC, RC), Name);
  }
  return InsertHelper(BinaryOperator::Create(Opcode::Add, LHS, RHS, CurBB),
                      Name);
}

std::shared_ptr<Value> ModuleBuilder::CreateSub(std::shared_ptr<Value> LHS, std::shared_ptr<Value> RHS, std::string Name) {
  if (std::shared_ptr<ConstantInt> LC = std::dynamic_pointer_cast<ConstantInt>(LHS)) {
    if (std::shared_ptr<ConstantInt> RC = std::dynamic_pointer_cast<ConstantInt>(RHS))
      return InsertHelper(
          ConstantFolder::CreateArithmetic(Context, Opcode::Sub, LC, RC), Name);
  }
  return InsertHelper(BinaryOperator::Create(Opcode::Sub, LHS, RHS, CurBB),
                      Name);
}

std::shared_ptr<Value> ModuleBuilder::CreateMul(std::shared_ptr<Value> LHS, std::shared_ptr<Value> RHS, std::string Name) {
  if (std::shared_ptr<ConstantInt> LC = std::dynamic_pointer_cast<ConstantInt>(LHS)) {
    if (std::shared_ptr<ConstantInt> RC = std::dynamic_pointer_cast<ConstantInt>(RHS))
      return InsertHelper(
          ConstantFolder::CreateArithmetic(Context, Opcode::Mul, LC, RC), Name);
  }
  return InsertHelper(BinaryOperator::Create(Opcode::Mul, LHS, RHS, CurBB),
                      Name);
}

std::shared_ptr<Value> ModuleBuilder::CreateDiv(std::shared_ptr<Value> LHS, std::shared_ptr<Value> RHS, std::string Name) {
  if (std::shared_ptr<ConstantInt> LC = std::dynamic_pointer_cast<ConstantInt>(LHS)) {
    if (std::shared_ptr<ConstantInt> RC = std::dynamic_pointer_cast<ConstantInt>(RHS))
      return InsertHelper(
          ConstantFolder::CreateArithmetic(Context, Opcode::Div, LC, RC), Name);
  }
  return InsertHelper(BinaryOperator::Create(Opcode::Div, LHS, RHS, CurBB),
                      Name);
}

std::shared_ptr<Value> ModuleBuilder::CreateRem(std::shared_ptr<Value> LHS, std::shared_ptr<Value> RHS, std::string Name) {
  if (std::shared_ptr<ConstantInt> LC = std::dynamic_pointer_cast<ConstantInt>(LHS)) {
    if (std::shared_ptr<ConstantInt> RC = std::dynamic_pointer_cast<ConstantInt>(RHS))
      return InsertHelper(
          ConstantFolder::CreateArithmetic(Context, Opcode::Rem, LC, RC), Name);
  }
  return InsertHelper(BinaryOperator::Create(Opcode::Rem, LHS, RHS, CurBB),
                      Name);
}

std::shared_ptr<Value> ModuleBuilder::CreateShl(std::shared_ptr<Value> LHS, std::shared_ptr<Value> RHS, std::string Name) {
  if (std::shared_ptr<ConstantInt> LC = std::dynamic_pointer_cast<ConstantInt>(LHS)) {
    if (std::shared_ptr<ConstantInt> RC = std::dynamic_pointer_cast<ConstantInt>(RHS))
      return InsertHelper(
          ConstantFolder::CreateArithmetic(Context, Opcode::Shl, LC, RC), Name);
  }
  return InsertHelper(BinaryOperator::Create(Opcode::Shl, LHS, RHS, CurBB),
                      Name);
}

std::shared_ptr<Value> ModuleBuilder::CreateShr(std::shared_ptr<Value> LHS, std::shared_ptr<Value> RHS, std::string Name) {
  if (std::shared_ptr<ConstantInt> LC = std::dynamic_pointer_cast<ConstantInt>(LHS)) {
    if (std::shared_ptr<ConstantInt> RC = std::dynamic_pointer_cast<ConstantInt>(RHS))
      return InsertHelper(
          ConstantFolder::CreateArithmetic(Context, Opcode::Shr, LC, RC), Name);
  }
  return InsertHelper(BinaryOperator::Create(Opcode::Shr, LHS, RHS, CurBB),
                      Name);
}

std::shared_ptr<Value> ModuleBuilder::CreateAnd(std::shared_ptr<Value> LHS, std::shared_ptr<Value> RHS, std::string Name) {
  if (std::shared_ptr<ConstantBool> LC = std::dynamic_pointer_cast<ConstantBool>(LHS)) {
    if (std::shared_ptr<ConstantBool> RC = std::dynamic_pointer_cast<ConstantBool>(RHS))
      return InsertHelper(
          ConstantFolder::CreateBoolean(Context, Opcode::And, LC, RC), Name);
  }
  return InsertHelper(BinaryOperator::Create(Opcode::And, LHS, RHS, CurBB),
                      Name);
}

std::shared_ptr<Value> ModuleBuilder::CreateOr(std::shared_ptr<Value> LHS, std::shared_ptr<Value> RHS, std::string Name) {
  if (std::shared_ptr<ConstantBool> LC = std::dynamic_pointer_cast<ConstantBool>(LHS)) {
    if (std::shared_ptr<ConstantBool> RC = std::dynamic_pointer_cast<ConstantBool>(RHS))
      return InsertHelper(
          ConstantFolder::CreateBoolean(Context, Opcode::Or, LC, RC), Name);
  }
  return InsertHelper(BinaryOperator::Create(Opcode::Or, LHS, RHS, CurBB),
                      Name);
}

std::shared_ptr<Value> ModuleBuilder::CreateNeg(std::shared_ptr<Value> V, std::string Name) {
  if (std::shared_ptr<ConstantInt> VC = std::dynamic_pointer_cast<ConstantInt>(V))
    return InsertHelper(ConstantFolder::CreateNeg(Context, VC), Name);
  return InsertHelper(BinaryOperator::CreateNeg(Context, V, CurBB));
}

std::shared_ptr<Value> ModuleBuilder::CreateNot(std::shared_ptr<Value> V, std::string Name) {
  if (std::shared_ptr<ConstantBool> VC = std::dynamic_pointer_cast<ConstantBool>(V))
    return InsertHelper(ConstantFolder::CreateNot(Context, VC), Name);
  return InsertHelper(BinaryOperator::CreateNot(Context, V, CurBB));
}

//===------------------------------------------------------------------===//
// Instruction creation methods: Memory Instructions.
std::shared_ptr<AllocaInst>  ModuleBuilder::CreateAlloca(TyPtr Ty, std::string Name) {
  return InsertHelper(AllocaInst::Create(Ty, CurBB), Name);
}

std::shared_ptr<LoadInst> ModuleBuilder::CreateLoad(std::shared_ptr<Value> Ptr) {
  return InsertHelper(LoadInst::Create(Ptr, CurBB));
}

std::shared_ptr<StoreInst> ModuleBuilder::CreateStore(std::shared_ptr<Value> Val, std::shared_ptr<Value> Ptr) {
  return InsertHelper(StoreInst::Create(Context, Val, Ptr, CurBB), "");
}

std::shared_ptr<GetElementPtrInst> ModuleBuilder::CreateGEP([[maybe_unused]] TyPtr Ty, [[maybe_unused]] std::shared_ptr<Value> Ptr,
                                    std::vector<unsigned> IdxList,
                                    [[maybe_unused]] std::string Name) {
  std::vector<std::shared_ptr<Value>> ValPtrIdxList;
  // compute the std::shared_ptr<Value> of Indices.
  for (auto item : IdxList) {
    ValPtrIdxList.push_back(ConstantInt::get(Context, item));
  }
  // return InsertHelper(GetElementPtrInst::Create(Ty, Ptr, IdxList), Name);
  return nullptr;
}

std::shared_ptr<GetElementPtrInst> ModuleBuilder::CreateGEP(TyPtr Ty, std::shared_ptr<Value> Ptr, std::size_t Idx,
                                    std::string Name) {
  return InsertHelper(GetElementPtrInst::Create(
      Ty, Ptr, ConstantInt::getZeroValueForNegative(Context),
      ConstantInt::get(Context, (int)Idx), CurBB, Name));
}

//===--------------------------------------------------------------===//
// Instruction creation methods: Compare Instruction.
std::shared_ptr<Value> ModuleBuilder::CreateCmpEQ(std::shared_ptr<Value> LHS, std::shared_ptr<Value> RHS, std::string Name) {
  return InsertHelper(
      CmpInst::Create(Context, CmpInst::CMP_EQ, LHS, RHS, CurBB, Name), Name);
}

std::shared_ptr<Value> ModuleBuilder::CreateCmpNE(std::shared_ptr<Value> LHS, std::shared_ptr<Value> RHS, std::string Name) {
  return InsertHelper(
      CmpInst::Create(Context, CmpInst::CMP_NE, LHS, RHS, CurBB, Name), Name);
}

std::shared_ptr<Value> ModuleBuilder::CreateCmpGT(std::shared_ptr<Value> LHS, std::shared_ptr<Value> RHS, std::string Name) {
  return InsertHelper(
      CmpInst::Create(Context, CmpInst::CMP_GT, LHS, RHS, CurBB, Name), Name);
}

std::shared_ptr<Value> ModuleBuilder::CreateCmpGE(std::shared_ptr<Value> LHS, std::shared_ptr<Value> RHS, std::string Name) {
  return InsertHelper(
      CmpInst::Create(Context, CmpInst::CMP_GE, LHS, RHS, CurBB, Name), Name);
}

std::shared_ptr<Value> ModuleBuilder::CreateCmpLT(std::shared_ptr<Value> LHS, std::shared_ptr<Value> RHS, std::string Name) {
  return InsertHelper(
      CmpInst::Create(Context, CmpInst::CMP_LT, LHS, RHS, CurBB, Name), Name);
}

std::shared_ptr<Value> ModuleBuilder::CreateCmpLE(std::shared_ptr<Value> LHS, std::shared_ptr<Value> RHS, std::string Name) {
  return InsertHelper(
      CmpInst::Create(Context, CmpInst::CMP_LE, LHS, RHS, CurBB, Name), Name);
}

std::shared_ptr<Value> ModuleBuilder::CreateCmp(CmpInst::Predicate P, std::shared_ptr<Value> LHS, std::shared_ptr<Value> RHS,
                                std::string Name) {
  // ���ж��ܷ����constant folder
  if (true) {
  }
  return InsertHelper(CmpInst::Create(Context, P, LHS, RHS, CurBB), Name);
}

//===-----------------------------------------------------------------===//
// Instruction creation methods: Other Instructions.
std::shared_ptr<PHINode> ModuleBuilder::CreatePHI(TyPtr Ty, unsigned NumReservedValues,
                                    std::string Name) {
  return InsertHelper(PHINode::Create(Ty, NumReservedValues), Name);
}

std::shared_ptr<CallInst> ModuleBuilder::CreateCall(std::shared_ptr<Value> Callee, std::vector<std::shared_ptr<Value>> Args) {
  return InsertHelper(CallInst::Create(Callee, Args, CurBB));
}

std::shared_ptr<CallInst> ModuleBuilder::CreateIntrinsic(std::shared_ptr<Intrinsic> Intr,
                                           std::vector<std::shared_ptr<Value>> Args) {
  return InsertHelper(CallInst::Create(Intr, Args, CurBB));
}
std::shared_ptr<ExtractValueInst> ModuleBuilder::CreateExtractValueValue(std::shared_ptr<Value> Agg,
                                                 std::vector<unsigned> Idxs,
                                                 std::string Name) {
  // To Do: Constant folder
  if (ConstantPtr AggC = std::dynamic_pointer_cast<Constant>(Agg)) {
  }
  return InsertHelper(ExtractValueInst::Create(Agg, Idxs), Name);
}

//===-----------------------------------------------------------------===//
// Utility creation methods.
std::shared_ptr<Value> ModuleBuilder::CreateIsNull([[maybe_unused]] std::shared_ptr<Value> Arg, [[maybe_unused]] std::string Name) {
  // To Do:
  // return CreateCmpEQ(Arg, ), Name);
  return nullptr;
}

/// \brief Return an i1 value testing if \p Arg is not null.
std::shared_ptr<Value> ModuleBuilder::CreateIsNotNull([[maybe_unused]] std::shared_ptr<Value> Arg, [[maybe_unused]] std::string Name) {
  // return CreateCmpNE(Arg, Constant::getNullValue(Arg->getType()), Name);
  return nullptr;
}

//===-------------------------------------------------------------------===//
// Other Create* helper.
std::shared_ptr<BasicBlock> ModuleBuilder::CreateBasicBlock(std::string Name, std::shared_ptr<Function> parent,
                                      std::shared_ptr<BasicBlock> before) {
  return BasicBlock::Create(Name, parent, before);
}

//===-------------------------------------------------------------------===//
void ModuleBuilder::SaveTopLevelCtxInfo() {
  CurFunc->TopLevelAllocaIsPt = AllocaInsertPoint;
  CurFunc->TopLevelCurBB = CurBB;
  CurFunc->TopLevelEntry = EntryBlock;
  CurFunc->TopLevelCurIsPt = InsertPoint;
  CurFunc->TopLevelIsAllocaInsertPointSetByNormalInsert =
      isAllocaInsertPointSetByNormalInsert;
  CurFunc->TopLevelTempCounter = TempCounter;
}

void ModuleBuilder::RestoreTopLevelCtxInfo() {
  AllocaInsertPoint = CurFunc->TopLevelAllocaIsPt;
  CurBB = CurFunc->TopLevelCurBB;
  EntryBlock = CurFunc->TopLevelEntry;
  InsertPoint = CurFunc->TopLevelCurIsPt;
  isAllocaInsertPointSetByNormalInsert =
      CurFunc->TopLevelIsAllocaInsertPointSetByNormalInsert;
  TempCounter = CurFunc->TopLevelTempCounter;
}
void print([[maybe_unused]] std::shared_ptr<Value> V) {
  std::ostringstream out;
  cout << out.str();
}
