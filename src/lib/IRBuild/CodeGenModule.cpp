//===--------------------------CodeGenMo
#include "../../include/IRBuild/IRBuilder.h"
using namespace compiler::IRBuild;
using namespace compiler::IR;

//===---------------------------------------------------------------------===//
// Implements class ModuleBuilder.
ModuleBuilder::ModuleBuilder(std::shared_ptr<Scope> SymbolInfo, MosesIRContext &context) : 
	SymbolTree(SymbolInfo), Context(context), Types(CodeGenTypes(context)), 
	CurBB(std::make_shared<BasicBlock>("entry")), CurFunc(std::make_shared<FunctionBuilderStatus>()),
	CurScope(SymbolInfo)
{
	SetInsertPoint(CurBB);
	// Create a marker to make it easy to insert allocas into the entryblock later.
	AllocaInsertPoint = InsertPoint;
}

void ModuleBuilder::VisitChildren(std::vector<std::shared_ptr<StatementAST>> AST)
{
	unsigned ASTSize = AST.size();
	for (unsigned i = 0; i < ASTSize; i++)
	{
		// 在遍历AST的过程中，会根据AST的具体节点来选择对应的Accept函数。
		// 例如：如果这里是 "IfStmt" 的话，会选择IfStmt的Accept()函数
		AST[i].get()->Accept(this);
	}
}

//===---------------------------------------------------------------------===//
// Helper for CodeGen.
//TyPtr ModuleBuilder::ConvertTypeForMem(std::shared_ptr<compiler::ast::Type> T)
//{
//	TyPtr ResultType = nullptr;
//	switch (T->getKind())
//	{
//	case compiler::ast::TypeKind::INT:
//		break;
//	case compiler::ast::TypeKind::BOOL:
//		break;
//	case compiler::ast::TypeKind::ANONYMOUS:
//		break;
//	case compiler::ast::TypeKind::USERDEFIED:
//		break;
//	case compiler::ast::TypeKind::VOID:
//		break;
//	default:
//		break;
//	}
//	return nullptr;
//}

//===---------------------------------------------------------------------===//
// private helper method.

void ModuleBuilder::SetInsertPoint(BBPtr TheBB)
{
	CurBB = TheBB;
	InsertPoint = CurBB->end();
}

void ModuleBuilder::SetInsertPoint(InstPtr I)
{
	CurBB = I->getParent();
	InsertPoint = CurBB->getIterator(I);
}

//===---------------------------------------------------------------------===//
// Instruction creation methods: Terminators.
ReturnInstPtr ModuleBuilder::CreateRetVoid() 
{ 
	return InsertHelper(ReturnInst::Create()); 
}

ReturnInstPtr ModuleBuilder::CreateRet(ValPtr V)
{
	return InsertHelper(ReturnInst::Create(V));
}

ReturnInstPtr ModuleBuilder::CreateAggregateRet(std::vector<ValPtr> retVals, unsigned N)
{
	return nullptr;
}

BrInstPtr ModuleBuilder::Create(BBPtr Dest)
{
	return InsertHelper(BranchInst::Create(Dest));
}

BrInstPtr ModuleBuilder::CreateCondBr(ValPtr Cond, BBPtr True, BBPtr False)
{
	return InsertHelper(BranchInst::Create(True, False, Cond));
}

BrInstPtr ModuleBuilder::CreateBr(BBPtr Dest)
{
	return InsertHelper(BranchInst::Create(Dest));
}

//===-------------------------------------------------------------===//
// Instruction creation methods: Binary Operators
BOInstPtr ModuleBuilder::CreateInsertBinOp(BinaryOperator::Opcode Opc, ValPtr LHS, ValPtr RHS,
	std::string Name)
{
	BOInstPtr BO = InsertHelper(BinaryOperator::Create(Opc, LHS, RHS), Name);
	return BO;
}

ValPtr ModuleBuilder::CreateAdd(ValPtr LHS, ValPtr RHS, std::string Name)
{
	if (ConstantIntPtr LC = std::dynamic_pointer_cast<ConstantInt>(LHS))
	{
		if (ConstantIntPtr RC = std::dynamic_pointer_cast<ConstantInt>(RHS))
			return InsertHelper(ConstantFolder::CreateArithmetic(Context, Opcode::Add, LC, RC), Name);
	}
	return InsertHelper(BinaryOperator::Create(Opcode::Add, LHS, RHS), Name);
}

ValPtr ModuleBuilder::CreateSub(ValPtr LHS, ValPtr RHS, std::string Name)
{
	if (ConstantIntPtr LC = std::dynamic_pointer_cast<ConstantInt>(LHS))
	{
		if (ConstantIntPtr RC = std::dynamic_pointer_cast<ConstantInt>(RHS))
			return InsertHelper(ConstantFolder::CreateArithmetic(Context, Opcode::Sub, LC, RC), Name);
	}
	return InsertHelper(BinaryOperator::Create(Opcode::Sub, LHS, RHS), Name);
}

ValPtr ModuleBuilder::CreateMul(ValPtr LHS, ValPtr RHS, std::string Name)
{
	if (ConstantIntPtr LC = std::dynamic_pointer_cast<ConstantInt>(LHS))
	{
		if (ConstantIntPtr RC = std::dynamic_pointer_cast<ConstantInt>(RHS))
			return InsertHelper(ConstantFolder::CreateArithmetic(Context, Opcode::Mul, LC, RC), Name);
	}
	return InsertHelper(BinaryOperator::Create(Opcode::Mul, LHS, RHS), Name);
}

ValPtr ModuleBuilder::CreateDiv(ValPtr LHS, ValPtr RHS, std::string Name)
{
	if (ConstantIntPtr LC = std::dynamic_pointer_cast<ConstantInt>(LHS))
	{
		if (ConstantIntPtr RC = std::dynamic_pointer_cast<ConstantInt>(RHS))
			return InsertHelper(ConstantFolder::CreateArithmetic(Context, Opcode::Div, LC, RC), Name);
	}
	return InsertHelper(BinaryOperator::Create(Opcode::Div, LHS, RHS), Name);
}

ValPtr ModuleBuilder::CreateRem(ValPtr LHS, ValPtr RHS, std::string Name)
{
	if (ConstantIntPtr LC = std::dynamic_pointer_cast<ConstantInt>(LHS))
	{
		if (ConstantIntPtr RC = std::dynamic_pointer_cast<ConstantInt>(RHS))
			return InsertHelper(ConstantFolder::CreateArithmetic(Context, Opcode::Rem, LC, RC), Name);
	}
	return InsertHelper(BinaryOperator::Create(Opcode::Rem, LHS, RHS), Name);
}

ValPtr ModuleBuilder::CreateShl(ValPtr LHS, ValPtr RHS, std::string Name)
{
	if (ConstantIntPtr LC = std::dynamic_pointer_cast<ConstantInt>(LHS))
	{
		if (ConstantIntPtr RC = std::dynamic_pointer_cast<ConstantInt>(RHS))
			return InsertHelper(ConstantFolder::CreateArithmetic(Context, Opcode::Shl, LC, RC), Name);
	}
	return InsertHelper(BinaryOperator::Create(Opcode::Shl, LHS, RHS), Name);
}

ValPtr ModuleBuilder::CreateShr(ValPtr LHS, ValPtr RHS, std::string Name)
{
	if (ConstantIntPtr LC = std::dynamic_pointer_cast<ConstantInt>(LHS))
	{
		if (ConstantIntPtr RC = std::dynamic_pointer_cast<ConstantInt>(RHS))
			return InsertHelper(ConstantFolder::CreateArithmetic(Context, Opcode::Shr, LC, RC), Name);
	}
	return InsertHelper(BinaryOperator::Create(Opcode::Shr, LHS, RHS), Name);
}

ValPtr ModuleBuilder::CreateAnd(ValPtr LHS, ValPtr RHS, std::string Name)
{
	if (ConstantBoolPtr LC = std::dynamic_pointer_cast<ConstantBool>(LHS))
	{
		if (ConstantBoolPtr RC = std::dynamic_pointer_cast<ConstantBool>(RHS))
			return InsertHelper(ConstantFolder::CreateBoolean(Context, Opcode::And, LC, RC), Name);
	}
	return InsertHelper(BinaryOperator::Create(Opcode::And, LHS, RHS), Name);
}

ValPtr ModuleBuilder::CreateOr(ValPtr LHS, ValPtr RHS, std::string Name)
{
	if (ConstantBoolPtr LC = std::dynamic_pointer_cast<ConstantBool>(LHS))
	{
		if (ConstantBoolPtr RC = std::dynamic_pointer_cast<ConstantBool>(RHS))
			return InsertHelper(ConstantFolder::CreateBoolean(Context, Opcode::Or, LC, RC), Name);
	}
	return InsertHelper(BinaryOperator::Create(Opcode::Or, LHS, RHS), Name);
}

ValPtr ModuleBuilder::CreateNeg(ValPtr V, std::string Name)
{
	if (ConstantIntPtr VC = std::dynamic_pointer_cast<ConstantInt>(V))
		return InsertHelper(ConstantFolder::CreateNeg(Context, VC), Name);
	return InsertHelper(BinaryOperator::CreateNeg(Context, V));
}

ValPtr ModuleBuilder::CreateNot(ValPtr V, std::string Name)
{
	if (ConstantBoolPtr VC = std::dynamic_pointer_cast<ConstantBool>(V))
		return InsertHelper(ConstantFolder::CreateNot(Context, VC), Name);
	return InsertHelper(BinaryOperator::CreateNot(Context, V));
}

//===------------------------------------------------------------------===//
// Instruction creation methods: Memory Instructions.
AllocaInstPtr ModuleBuilder::CreateAlloca(TyPtr Ty, std::string Name)
{
	return InsertHelper(AllocaInst::Create(Ty));
}

LoadInstPtr ModuleBuilder::CreateLoad(ValPtr Ptr)
{
	return InsertHelper(LoadInst::Create(Ptr));
}

StoreInstPtr ModuleBuilder::CreateStore(ValPtr Val, ValPtr Ptr)
{
	return InsertHelper(StoreInst::Create(Val, Ptr));
}

GEPInstPtr ModuleBuilder::CreateGEP(TyPtr Ty, ValPtr Ptr, std::vector<ValPtr> IdxList, std::string Name)
{
	// 判断地址计算能否预先计算完成
	if (true)
	{

	}
	return InsertHelper(GetElementPtrInst::Create(Ty, Ptr, IdxList), Name);
}

//===--------------------------------------------------------------===//
// Instruction creation methods: Compare Instruction.
ValPtr ModuleBuilder::CreateCmpEQ(ValPtr LHS, ValPtr RHS, std::string Name)
{
	return CmpInst::Create(CmpInst::CMP_EQ, LHS, RHS, Name);
}

ValPtr ModuleBuilder::CreateCmpNE(ValPtr LHS, ValPtr RHS, std::string Name)
{
	return CmpInst::Create(CmpInst::CMP_NE, LHS, RHS, Name);
}

ValPtr ModuleBuilder::CreateCmpGT(ValPtr LHS, ValPtr RHS, std::string Name)
{
	return CmpInst::Create(CmpInst::CMP_GT, LHS, RHS, Name);
}

ValPtr ModuleBuilder::CreateCmpGE(ValPtr LHS, ValPtr RHS, std::string Name)
{
	return CmpInst::Create(CmpInst::CMP_GE, LHS, RHS, Name);
}

ValPtr ModuleBuilder::CreateCmpLT(ValPtr LHS, ValPtr RHS, std::string Name)
{
	return CmpInst::Create(CmpInst::CMP_LT, LHS, RHS, Name);
}

ValPtr ModuleBuilder::CreateCmpLE(ValPtr LHS, ValPtr RHS, std::string Name)
{
	return CmpInst::Create(CmpInst::CMP_LE, LHS, RHS, Name);
}

ValPtr ModuleBuilder::CreateCmp(CmpInst::Predicate P, ValPtr LHS, ValPtr RHS, std::string Name)
{
	// 先判断能否进行constant folder
	if (true)
	{
	}
	return InsertHelper(CmpInst::Create(P, LHS, RHS), Name);
}

//===-----------------------------------------------------------------===//
// Instruction creation methods: Other Instructions.
PHINodePtr ModuleBuilder::CreatePHI(TyPtr Ty, unsigned NumReservedValues, std::string Name)
{
	return InsertHelper(PHINode::Create(Ty, NumReservedValues), Name);
}

CallInstPtr ModuleBuilder::CreateCall(ValPtr Callee, std::vector<ValPtr> Args, std::string Name)
{
	Callee->getType();
	return InsertHelper(CallInst::Create(Callee, Args), Name);
}

EVInstPtr ModuleBuilder::CreateExtractValueValue(ValPtr Agg, std::vector<unsigned> Idxs, std::string Name)
{
	// Constant folder
	if (ConstantPtr AggC = std::dynamic_pointer_cast<Constant>(Agg))
	{
		// folder
	}
	return InsertHelper(ExtractValueInst::Create(Agg, Idxs), Name);
}

//===-----------------------------------------------------------------===//
// Utility creation methods.
ValPtr ModuleBuilder::CreateIsNull(ValPtr Arg, std::string Name)
{
	// To Do:
	// return CreateCmpEQ(Arg, ), Name);
	return nullptr;
}

/// \brief Return an i1 value testing if \p Arg is not null.
ValPtr ModuleBuilder::CreateIsNotNull(ValPtr Arg, std::string Name)
{
	// return CreateCmpNE(Arg, Constant::getNullValue(Arg->getType()), Name);
	return nullptr;
}

//===-------------------------------------------------------------------===//
// Other Create* helper.
BBPtr ModuleBuilder::CreateBasicBlock(std::string Name, FuncPtr parent, BBPtr before)
{
	return BasicBlock::Create(Name, parent, before);
}