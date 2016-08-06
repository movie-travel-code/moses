//===------------------------ExecutionEngine.cpp--------------------------===//
//
// Impletes the class ExecutionEngine.
//
//===---------------------------------------------------------------------===//
#include "../../include/ExecutionEngine/ExecutionEngine.h"

using namespace compiler::Interpreter;

Interpreter::Interpreter(std::vector<ValPtr> &Insts, const MosesIRContext &Ctx):
	Insts(Insts), Ctx(Ctx)
{
	// Initialize the top-level-frame.
	ECStack.push_back(ExecutionContext());

	ExecutionContext &SF = ECStack.back();
	// nullptr means top-level-frame.
	SF.CurFunction = nullptr;
	// Get the first BasicBlock to execute.
	for (auto val : Insts)
	{
		if (BBPtr BB = std::dynamic_pointer_cast<BasicBlock>(val))
		{
			SF.CurBB = BB;
			break;
		}
	}
	SF.CurInst = SF.CurBB->begin();
	
	// Start executing the function call.
	run();
}

/// create - Create an interpreter ExecutionEngine. This can never fail.
std::shared_ptr<Interpreter> create(std::vector<ValPtr> &Insts, const MosesIRContext &Ctx)
{
	return std::make_shared<Interpreter>(Insts, Ctx);
}

void Interpreter::run()
{
	while (!ECStack.empty())
	{
		// Interprete a single instruction & increment the "PC"
		// Current stack frame.
		ExecutionContext &SF = ECStack.back();
		InstPtr I = SF.IssueInstruction();
		visit(I);
	}
}

void Interpreter::visit(InstPtr I)
{
	switch (I->getOpcode())
	{
	case Opcode::Add:
	case Opcode::Sub:
	case Opcode::Mul:
	case Opcode::Div:
	case Opcode::Shl:
	case Opcode::Shr:
	case Opcode::Or:
	case Opcode::And:	
	case Opcode::Rem:
	{
		auto BOInst = std::dynamic_pointer_cast<BinaryOperator>(I);	
		assert(BOInst && "Instruction's kind is wrong");
		visitBinaryOperator(BOInst);
		break;
	}		
	case Opcode::Alloca:
	{
		auto AllocaI = std::dynamic_pointer_cast<AllocaInst>(I);
		assert(AllocaI && "Instruction's kind is wrong");
		visitAllocaInst(AllocaI);
		break;
	}		
	case Opcode::Br:
	{
		auto BrInst = std::dynamic_pointer_cast<BranchInst>(I);
		assert(BrInst && "Instruction's kind is wrong");
		visitBranchInst(BrInst);
		break;
	}
	case Opcode::Call:
	{
		auto CallI = std::dynamic_pointer_cast<CallInst>(I);	
		assert(CallI && "Instruction's kind is wrong");
		visitCallInst(CallI);
		break;
	}
	case Opcode::Cmp:
	{
		auto CMPI = std::dynamic_pointer_cast<CmpInst>(I);
		assert(CMPI && "Instruction's kind is wrong.");
		visitCmpInst(CMPI);
		break;
	}
	case Opcode::GetElementPtr:
	{
		auto GEPI = std::dynamic_pointer_cast<GetElementPtrInst>(I);
		assert(GEPI && "Instruction's kind is wrong.");
		visitGEPInst(GEPI);
		break;
	}
	case Opcode::Load:
		break;	
	case Opcode::Store:
		break;
	case Opcode::Xor:
		break;
	case Opcode::Ret:
		break;	
	case Opcode::PHI:
		break;
	default:
		break;
	}
}

void Interpreter::visitBinaryOperator(BOInstPtr I)
{
	ExecutionContext &SF = ECStack.back();
	TyPtr Ty = I->getType();
	GenericValue Src1 = getOperandValue(I->getOperand(0).get(), SF);
	GenericValue Src2 = getOperandValue(I->getOperand(1).get(), SF);
	GenericValue R;

	switch (I->getOpcode())
	{
	case Opcode::Add: R.IntVal = Src1.IntVal + Src2.IntVal; break;
	case Opcode::Sub: R.IntVal = Src1.IntVal - Src2.IntVal; break;
	case Opcode::Mul: R.IntVal = Src1.IntVal * Src2.IntVal; break;
	case Opcode::Div: R.IntVal = Src1.IntVal / Src2.IntVal; break;
	case Opcode::And: R.BoolVal = Src1.BoolVal && Src2.BoolVal; break;
	case Opcode::Or: R.BoolVal = Src1.BoolVal || Src2.BoolVal; break;
	case Opcode::Rem: R.IntVal = Src1.IntVal % Src2.IntVal; break;
	case Opcode::Shr: R.IntVal = Src1.IntVal >> Src2.IntVal; break;
	case Opcode::Shl: R.IntVal = Src1.IntVal << Src2.IntVal; break;
	default:
		assert(0 && "Unreachable code.");
	}
}

/// visitAllocaInst - Alloca the memory and update the information.
void Interpreter::visitAllocaInst(AllocaInstPtr I)
{
	ExecutionContext &SF = ECStack.back();
	auto PTy = std::dynamic_pointer_cast<PointerType>(I->getType());
	assert(PTy && "PTy is null.");
	auto ty = PTy->getElementTy();

	unsigned TypeSize = ty->getSize();

	// Alloca enough memory to hold the type...
	void *Memory = malloc(TypeSize);
	GenericValue Result = PTOGV(Memory);
	ECStack.back().Allocas.add(Memory);
}

void Interpreter::visitBranchInst(BrInstPtr I)
{
	ExecutionContext &SF = ECStack.back();
	auto Dest = I->getSuccessor(0);
	if (!I->isUncoditional())
	{
		GenericValue CondV = getOperandValue(I->getCondition(), SF);
		if (!CondV.BoolVal)
			Dest = I->getSuccessor(1);
	}
	SwitchToNewBasicBlock(Dest, SF);
}

void Interpreter::visitCallInst(CallInstPtr I)
{
	ExecutionContext &SF = ECStack.back();
	// Check the current call is Intrinsic.
	if (I->isIntrinsicCall())
	{
	}
	
	SF.Caller = I;
	// prepare the arg values.
	std::vector<GenericValue> ArgVals;
	for (unsigned i = 0, size = I->getNumArgOperands(); i < size; i++)
		ArgVals.push_back(getOperandValue(I->getArgOperand(i), SF));

	auto Func = std::dynamic_pointer_cast<Function>(I->getOperand(0).get());
	assert(I && "Call instruciton's function can't be null.");
	callFunction(Func, ArgVals);
}

void Interpreter::visitCmpInst(CmpInstPtr I)
{
	ExecutionContext &SF = ECStack.back();
	TyPtr Ty = I->getOperand(0).get()->getType();

	CmpInst::Predicate PreD = I->getPredicate();
	GenericValue LHS = getOperandValue(I->getOperand(0).get(), SF);
	GenericValue RHS = getOperandValue(I->getOperand(1).get(), SF);
	GenericValue Result;

	switch (PreD)
	{
	case compiler::IR::CmpInst::CMP_EQ:
		if (Ty->isBoolTy()) Result.BoolVal = LHS.BoolVal == RHS.BoolVal;
		if(Ty->isIntegerTy()) Result.BoolVal = LHS.IntVal == RHS.IntVal;
		// To Do: Userdefined type
		break;
	case compiler::IR::CmpInst::CMP_NE:
		if (Ty->isBoolTy()) Result.BoolVal = LHS.BoolVal != RHS.BoolVal;
		if (Ty->isIntegerTy()) Result.IntVal = LHS.IntVal != RHS.IntVal;
		break;
	case compiler::IR::CmpInst::CMP_GT:
		Result.BoolVal = LHS.IntVal > RHS.IntVal;
		break;
	case compiler::IR::CmpInst::CMP_GE:
		Result.BoolVal = LHS.IntVal >= RHS.IntVal;
		break;
	case compiler::IR::CmpInst::CMP_LT:
		Result.BoolVal = LHS.IntVal < RHS.IntVal;
		break;
	case compiler::IR::CmpInst::CMP_LE:
		Result.BoolVal = LHS.IntVal <= RHS.IntVal;
		break;
	default:
		assert(0 && "Unreachable code.");
		break;
	}
	SetGenericValue(I, Result, SF);
}

void Interpreter::visitGEPInst(GEPInstPtr I)
{
	ExecutionContext &SF = ECStack.back();
	void* BaseAddr = GVTOP(getOperandValue(I->getOperand(0).get(), SF));
	unsigned long TotalIndex = 0;
	GenericValue Result;
	// GEP instruction just have two operand. Shit.
	GenericValue index = getOperandValue(I->getOperand(1).get(), SF);
	
	TotalIndex = index.IntVal;
	Result.PointerVal = (char*)BaseAddr + index.IntVal * sizeof(int);
	SetGenericValue(I, Result, SF);
}

void Interpreter::SwitchToNewBasicBlock(BBPtr New, ExecutionContext &SF)
{
	SF.CurBB = New;
	SF.CurInst = New->begin();
	// Now have no phi node, so return.
}

GenericValue Interpreter::getConstantValue(ConstantPtr C)
{
	GenericValue Result;
	if (ConstantBoolPtr CB = std::dynamic_pointer_cast<ConstantBool>(C))
		Result.BoolVal = CB->getVal();
	
	if (ConstantIntPtr CI = std::dynamic_pointer_cast<ConstantInt>(C))
		Result.IntVal = CI->getVal();
	return Result;
}

GenericValue Interpreter::getOperandValue(ValPtr V, ExecutionContext &SF)
{
	if (ConstantPtr CPV = std::dynamic_pointer_cast<Constant>(V))
		return getConstantValue(CPV);
	else
		return getLocalAndGlobalGV(V);
}

GenericValue Interpreter::getLocalAndGlobalGV(ValPtr V)
{
	ExecutionContext &SF = ECStack.back();
	if (SF.Values.find(V) != SF.Values.end())
		return SF.Values[V];
	else
		return TopLevelFrame.Values[V];
}

void Interpreter::SetGenericValue(ValPtr V, GenericValue GV, ExecutionContext &SF)
{
	SF.Values[V] = GV;
}

void Interpreter::callFunction(FuncPtr Function, std::vector<GenericValue> ArgVals)
{
	// Create a new stack frame.
	ECStack.push_back(ExecutionContext());

	ExecutionContext &SF = ECStack.back();
	SF.CurFunction = Function;
	SF.CurBB = Function->getEntryBlock();
	SF.CurInst = SF.CurBB->begin();

	// This is the most interesting part.
	// Handle the argument passing.
	for (unsigned i = 0, size = Function->getArgumentList().size(); i < size; i++)
		SetGenericValue(Function->getArg(i), ArgVals[i], SF);
}