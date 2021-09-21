//===--------------------------CodeGenMo
#include "include/IRBuild/IRBuilder.h"
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
  unsigned ASTSize = AST.size();
  for (unsigned i = 0; i < ASTSize; i++)
    AST[i].get()->Accept(this);
}

//===---------------------------------------------------------------------===//
// Helper for CodeGen.

//===---------------------------------------------------------------------===//
// private helper method.

void ModuleBuilder::SetInsertPoint(BBPtr TheBB) {
  CurBB = TheBB;
  InsertPoint = CurBB->end();
}

void ModuleBuilder::SetInsertPoint(InstPtr I) {
  CurBB = I->getParent();
  InsertPoint = CurBB->getIterator(I);
}

//===---------------------------------------------------------------------===//
// Instruction creation methods: Terminators.
ReturnInstPtr ModuleBuilder::CreateRetVoid() {
  return InsertHelper(ReturnInst::Create(CurBB));
}

ReturnInstPtr ModuleBuilder::CreateRet(ValPtr V) {
  return InsertHelper(ReturnInst::Create(CurBB, V));
}

ReturnInstPtr ModuleBuilder::CreateAggregateRet(std::vector<ValPtr> retVals,
                                                unsigned N) {
  return nullptr;
}

BrInstPtr ModuleBuilder::Create(BBPtr Dest) {
  return InsertHelper(BranchInst::Create(Context, CurBB, Dest), "");
}

BrInstPtr ModuleBuilder::CreateCondBr(ValPtr Cond, BBPtr True, BBPtr False) {
  return InsertHelper(BranchInst::Create(Context, True, False, Cond, CurBB),
                      "");
}

BrInstPtr ModuleBuilder::CreateBr(BBPtr Dest) {
  return InsertHelper(BranchInst::Create(Context, Dest, CurBB), "");
}

//===-------------------------------------------------------------===//
// Instruction creation methods: Binary Operators
BOInstPtr ModuleBuilder::CreateInsertBinOp(BinaryOperator::Opcode Opc,
                                           ValPtr LHS, ValPtr RHS,
                                           std::string Name) {
  BOInstPtr BO =
      InsertHelper(BinaryOperator::Create(Opc, LHS, RHS, CurBB), Name);
  return BO;
}

ValPtr ModuleBuilder::CreateAdd(ValPtr LHS, ValPtr RHS, std::string Name) {
  if (ConstantIntPtr LC = std::dynamic_pointer_cast<ConstantInt>(LHS)) {
    if (ConstantIntPtr RC = std::dynamic_pointer_cast<ConstantInt>(RHS))
      return InsertHelper(
          ConstantFolder::CreateArithmetic(Context, Opcode::Add, LC, RC), Name);
  }
  return InsertHelper(BinaryOperator::Create(Opcode::Add, LHS, RHS, CurBB),
                      Name);
}

ValPtr ModuleBuilder::CreateSub(ValPtr LHS, ValPtr RHS, std::string Name) {
  if (ConstantIntPtr LC = std::dynamic_pointer_cast<ConstantInt>(LHS)) {
    if (ConstantIntPtr RC = std::dynamic_pointer_cast<ConstantInt>(RHS))
      return InsertHelper(
          ConstantFolder::CreateArithmetic(Context, Opcode::Sub, LC, RC), Name);
  }
  return InsertHelper(BinaryOperator::Create(Opcode::Sub, LHS, RHS, CurBB),
                      Name);
}

ValPtr ModuleBuilder::CreateMul(ValPtr LHS, ValPtr RHS, std::string Name) {
  if (ConstantIntPtr LC = std::dynamic_pointer_cast<ConstantInt>(LHS)) {
    if (ConstantIntPtr RC = std::dynamic_pointer_cast<ConstantInt>(RHS))
      return InsertHelper(
          ConstantFolder::CreateArithmetic(Context, Opcode::Mul, LC, RC), Name);
  }
  return InsertHelper(BinaryOperator::Create(Opcode::Mul, LHS, RHS, CurBB),
                      Name);
}

ValPtr ModuleBuilder::CreateDiv(ValPtr LHS, ValPtr RHS, std::string Name) {
  if (ConstantIntPtr LC = std::dynamic_pointer_cast<ConstantInt>(LHS)) {
    if (ConstantIntPtr RC = std::dynamic_pointer_cast<ConstantInt>(RHS))
      return InsertHelper(
          ConstantFolder::CreateArithmetic(Context, Opcode::Div, LC, RC), Name);
  }
  return InsertHelper(BinaryOperator::Create(Opcode::Div, LHS, RHS, CurBB),
                      Name);
}

ValPtr ModuleBuilder::CreateRem(ValPtr LHS, ValPtr RHS, std::string Name) {
  if (ConstantIntPtr LC = std::dynamic_pointer_cast<ConstantInt>(LHS)) {
    if (ConstantIntPtr RC = std::dynamic_pointer_cast<ConstantInt>(RHS))
      return InsertHelper(
          ConstantFolder::CreateArithmetic(Context, Opcode::Rem, LC, RC), Name);
  }
  return InsertHelper(BinaryOperator::Create(Opcode::Rem, LHS, RHS, CurBB),
                      Name);
}

ValPtr ModuleBuilder::CreateShl(ValPtr LHS, ValPtr RHS, std::string Name) {
  if (ConstantIntPtr LC = std::dynamic_pointer_cast<ConstantInt>(LHS)) {
    if (ConstantIntPtr RC = std::dynamic_pointer_cast<ConstantInt>(RHS))
      return InsertHelper(
          ConstantFolder::CreateArithmetic(Context, Opcode::Shl, LC, RC), Name);
  }
  return InsertHelper(BinaryOperator::Create(Opcode::Shl, LHS, RHS, CurBB),
                      Name);
}

ValPtr ModuleBuilder::CreateShr(ValPtr LHS, ValPtr RHS, std::string Name) {
  if (ConstantIntPtr LC = std::dynamic_pointer_cast<ConstantInt>(LHS)) {
    if (ConstantIntPtr RC = std::dynamic_pointer_cast<ConstantInt>(RHS))
      return InsertHelper(
          ConstantFolder::CreateArithmetic(Context, Opcode::Shr, LC, RC), Name);
  }
  return InsertHelper(BinaryOperator::Create(Opcode::Shr, LHS, RHS, CurBB),
                      Name);
}

ValPtr ModuleBuilder::CreateAnd(ValPtr LHS, ValPtr RHS, std::string Name) {
  if (ConstantBoolPtr LC = std::dynamic_pointer_cast<ConstantBool>(LHS)) {
    if (ConstantBoolPtr RC = std::dynamic_pointer_cast<ConstantBool>(RHS))
      return InsertHelper(
          ConstantFolder::CreateBoolean(Context, Opcode::And, LC, RC), Name);
  }
  return InsertHelper(BinaryOperator::Create(Opcode::And, LHS, RHS, CurBB),
                      Name);
}

ValPtr ModuleBuilder::CreateOr(ValPtr LHS, ValPtr RHS, std::string Name) {
  if (ConstantBoolPtr LC = std::dynamic_pointer_cast<ConstantBool>(LHS)) {
    if (ConstantBoolPtr RC = std::dynamic_pointer_cast<ConstantBool>(RHS))
      return InsertHelper(
          ConstantFolder::CreateBoolean(Context, Opcode::Or, LC, RC), Name);
  }
  return InsertHelper(BinaryOperator::Create(Opcode::Or, LHS, RHS, CurBB),
                      Name);
}

ValPtr ModuleBuilder::CreateNeg(ValPtr V, std::string Name) {
  if (ConstantIntPtr VC = std::dynamic_pointer_cast<ConstantInt>(V))
    return InsertHelper(ConstantFolder::CreateNeg(Context, VC), Name);
  return InsertHelper(BinaryOperator::CreateNeg(Context, V, CurBB));
}

ValPtr ModuleBuilder::CreateNot(ValPtr V, std::string Name) {
  if (ConstantBoolPtr VC = std::dynamic_pointer_cast<ConstantBool>(V))
    return InsertHelper(ConstantFolder::CreateNot(Context, VC), Name);
  return InsertHelper(BinaryOperator::CreateNot(Context, V, CurBB));
}

//===------------------------------------------------------------------===//
// Instruction creation methods: Memory Instructions.
AllocaInstPtr ModuleBuilder::CreateAlloca(TyPtr Ty, std::string Name) {
  return InsertHelper(AllocaInst::Create(Ty, CurBB), Name);
}

LoadInstPtr ModuleBuilder::CreateLoad(ValPtr Ptr) {
  return InsertHelper(LoadInst::Create(Ptr, CurBB));
}

StoreInstPtr ModuleBuilder::CreateStore(ValPtr Val, ValPtr Ptr) {
  return InsertHelper(StoreInst::Create(Context, Val, Ptr, CurBB), "");
}

GEPInstPtr ModuleBuilder::CreateGEP(TyPtr Ty, ValPtr Ptr,
                                    std::vector<unsigned> IdxList,
                                    std::string Name) {
  std::vector<ValPtr> ValPtrIdxList;
  // compute the ValPtr of Indices.
  for (auto item : IdxList) {
    ValPtrIdxList.push_back(ConstantInt::get(Context, item));
  }
  // return InsertHelper(GetElementPtrInst::Create(Ty, Ptr, IdxList), Name);
  return nullptr;
}

GEPInstPtr ModuleBuilder::CreateGEP(TyPtr Ty, ValPtr Ptr, unsigned Idx,
                                    std::string Name) {
  return InsertHelper(GetElementPtrInst::Create(
      Ty, Ptr, ConstantInt::getZeroValueForNegative(Context),
      ConstantInt::get(Context, Idx), CurBB, Name));
}

//===--------------------------------------------------------------===//
// Instruction creation methods: Compare Instruction.
ValPtr ModuleBuilder::CreateCmpEQ(ValPtr LHS, ValPtr RHS, std::string Name) {
  return InsertHelper(
      CmpInst::Create(Context, CmpInst::CMP_EQ, LHS, RHS, CurBB, Name), Name);
}

ValPtr ModuleBuilder::CreateCmpNE(ValPtr LHS, ValPtr RHS, std::string Name) {
  return InsertHelper(
      CmpInst::Create(Context, CmpInst::CMP_NE, LHS, RHS, CurBB, Name), Name);
}

ValPtr ModuleBuilder::CreateCmpGT(ValPtr LHS, ValPtr RHS, std::string Name) {
  return InsertHelper(
      CmpInst::Create(Context, CmpInst::CMP_GT, LHS, RHS, CurBB, Name), Name);
}

ValPtr ModuleBuilder::CreateCmpGE(ValPtr LHS, ValPtr RHS, std::string Name) {
  return InsertHelper(
      CmpInst::Create(Context, CmpInst::CMP_GE, LHS, RHS, CurBB, Name), Name);
}

ValPtr ModuleBuilder::CreateCmpLT(ValPtr LHS, ValPtr RHS, std::string Name) {
  return InsertHelper(
      CmpInst::Create(Context, CmpInst::CMP_LT, LHS, RHS, CurBB, Name), Name);
}

ValPtr ModuleBuilder::CreateCmpLE(ValPtr LHS, ValPtr RHS, std::string Name) {
  return InsertHelper(
      CmpInst::Create(Context, CmpInst::CMP_LE, LHS, RHS, CurBB, Name), Name);
}

ValPtr ModuleBuilder::CreateCmp(CmpInst::Predicate P, ValPtr LHS, ValPtr RHS,
                                std::string Name) {
  // ���ж��ܷ����constant folder
  if (true) {
  }
  return InsertHelper(CmpInst::Create(Context, P, LHS, RHS, CurBB), Name);
}

//===-----------------------------------------------------------------===//
// Instruction creation methods: Other Instructions.
PHINodePtr ModuleBuilder::CreatePHI(TyPtr Ty, unsigned NumReservedValues,
                                    std::string Name) {
  return InsertHelper(PHINode::Create(Ty, NumReservedValues), Name);
}

CallInstPtr ModuleBuilder::CreateCall(ValPtr Callee, std::vector<ValPtr> Args) {
  return InsertHelper(CallInst::Create(Callee, Args, CurBB));
}

CallInstPtr ModuleBuilder::CreateIntrinsic(IntrinsicPtr Intr,
                                           std::vector<ValPtr> Args) {
  return InsertHelper(CallInst::Create(Intr, Args, CurBB));
}
EVInstPtr ModuleBuilder::CreateExtractValueValue(ValPtr Agg,
                                                 std::vector<unsigned> Idxs,
                                                 std::string Name) {
  // To Do: Constant folder
  if (ConstantPtr AggC = std::dynamic_pointer_cast<Constant>(Agg)) {
  }
  return InsertHelper(ExtractValueInst::Create(Agg, Idxs), Name);
}

//===-----------------------------------------------------------------===//
// Utility creation methods.
ValPtr ModuleBuilder::CreateIsNull(ValPtr Arg, std::string Name) {
  // To Do:
  // return CreateCmpEQ(Arg, ), Name);
  return nullptr;
}

/// \brief Return an i1 value testing if \p Arg is not null.
ValPtr ModuleBuilder::CreateIsNotNull(ValPtr Arg, std::string Name) {
  // return CreateCmpNE(Arg, Constant::getNullValue(Arg->getType()), Name);
  return nullptr;
}

//===-------------------------------------------------------------------===//
// Other Create* helper.
BBPtr ModuleBuilder::CreateBasicBlock(std::string Name, FuncPtr parent,
                                      BBPtr before) {
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
void print(ValPtr V) {
  std::ostringstream out;
  cout << out.str();
}