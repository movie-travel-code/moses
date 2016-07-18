//===--------------------------Instruction.cpp----------------------------===//
// This file implements moses IR instructions.
//===---------------------------------------------------------------------===//
#include "../../include/IR/IRType.h"
#include "../../include/IR/Instruction.h"
#include "../../include/IR/MosesIRContext.h"

using namespace compiler::IR;
//===---------------------------------------------------------------------===//
// Implements Instruction.
Instruction::Instruction(TyPtr Ty, Opcode op) : 
	User(Ty, Value::ValueTy::InstructionVal)
{}

FuncPtr Instruction::getFunction() const { return Parent->getParent(); }

bool Instruction::mayReadFromMemory() const
{
	if (op == Opcode::Load)
		return true;
	return false;
}

bool Instruction::mayWriteToMemory() const
{
	if (op == Opcode::Store)
		return true;
	return false;
}
//Instruction::Instruction(TyPtr Ty, Opcode op, unsigned NumOps, BBPtr InsertAtEnd) : 
//	User(Ty, Value::ValueTy::InstructionVal)
//{
//
//}

//===---------------------------------------------------------------------===//
// Implements CmpInst.
CmpInst::CmpInst(InstPtr InsertBefore, Predicate pred, ValPtr LHS, ValPtr RHS, std::string NameStr) : 
	Instruction(nullptr, Opcode::Cmp)
{
	Operands.resize(2);
	Operands.push_back(Use(LHS, this));
	Operands.push_back(Use(RHS, this));
	// To Do: some checks.	
}

CmpInst::~CmpInst() {}

CmpInstPtr CmpInst::Create(Predicate predicate, ValPtr S1, ValPtr S2, std::string name, InstPtr InsertBefore)
{
	return std::make_shared<CmpInst>(nullptr, predicate, S1, S2, name);
}

CmpInstPtr CmpInst::Create(Predicate predicate, ValPtr S1, ValPtr S2,
	std::string name, BBPtr InsertAtEnd)
{
	return std::make_shared<CmpInst>(nullptr, predicate, S1, S2);
}

/// \brief Print CmpInst.
/// e.g.	if (parm > 0)
///			%tmp = load i32* %parm.addr
///			%cmp = cmp gt i32 %parm, 0			; <i1>
void CmpInst::Print(std::ostringstream& out)
{
	std::string PreStr;
	switch (predicate)
	{
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
	out << PreStr;
	Operands[0].get()->getType()->Print(out);
	out << " " << Operands[0].get()->getName() << ",";
	Operands[1].get()->getType()->Print(out);
	out << " " << Operands[0].get()->getName();
	out << "        ; < ";
	Type::getBoolType()->Print(out);
	out << " > \n";
}

//===---------------------------------------------------------------------===//
// Implements BinaryOperator Instructions.
BinaryOperator::BinaryOperator(Opcode op, ValPtr S1, ValPtr S2, TyPtr Ty, InstPtr InstructionBefore)
	: Instruction(Ty, op)
{
	Operands.resize(2);
	Operands.push_back(Use(S1, this));
	Operands.push_back(Use(S2, this));
	assert(S1 && S2 && S1->getType() == S2->getType());
}

BinaryOperator::~BinaryOperator() {}

BOInstPtr BinaryOperator::Create(Opcode op, ValPtr S1, ValPtr S2, BBPtr InsertAtEnd)
{
	assert(S1->getType() == S2->getType() &&
		"Cannot create binary operator with two operands of differing type!");
	return std::make_shared<BinaryOperator>(op, S1, S2, S1->getType());
}

BOInstPtr BinaryOperator::Create(Opcode op, ValPtr S1, ValPtr S2, InstPtr InsertBefore)
{
	return std::make_shared<BinaryOperator>(op, S1, S2, S1->getType());
}

BOInstPtr BinaryOperator::CreateNeg(MosesIRContext& Ctx, ValPtr Operand, InstPtr InsertBefore)
{
	ConstantIntPtr zero = ConstantInt::getZeroValueForNegative(Ctx);
	return Create(Opcode::Sub, zero, Operand, InsertBefore);
}

BOInstPtr BinaryOperator::CreateNeg(MosesIRContext& Ctx, ValPtr Operand, BBPtr InsertAtEnd)
{
	ConstantIntPtr zero = ConstantInt::getZeroValueForNegative(Ctx);
	return Create(Opcode::Sub, zero, Operand, InsertAtEnd);
}

BOInstPtr BinaryOperator::CreateNot(MosesIRContext& Ctx, ValPtr Operand, InstPtr InsertBefore)
{
	ConstantBoolPtr True = ConstantBool::getTrue(Ctx);
	return Create(Opcode::Xor, Operand, True, InsertBefore);
}

/// Shit code!
BOInstPtr BinaryOperator::CreateNot(MosesIRContext& Ctx, ValPtr Operand, BBPtr InsertAtEnd)
{
	ConstantBoolPtr True = ConstantBool::getTrue(Ctx);
	return Create(Opcode::Xor, Operand, True, InsertAtEnd);
}

/// \brief Print the BinaryOperator info.
/// e.g.	parm - 1
///			%sub = sub i32 %tmp1, 1
void BinaryOperator::Print(std::ostringstream& out)
{
	out << "%" << Name << " =";
	switch (op)
	{
	case compiler::IR::Instruction::Opcode::Add:
		out << " add";
		break;
	case compiler::IR::Instruction::Opcode::Sub:
		out << " sub";
		break;
	case compiler::IR::Instruction::Opcode::Mul:
		out << " mul";
		break;
	case compiler::IR::Instruction::Opcode::Div:
		out << " div";
		break;
	case compiler::IR::Instruction::Opcode::Rem:
		out << " rem";
		break;
	case compiler::IR::Instruction::Opcode::Shl:
		out << " shl";
		break;
	case compiler::IR::Instruction::Opcode::Shr:
		out << " shr";
		break;
	case compiler::IR::Instruction::Opcode::And:
		out << " and";
		break;
	case compiler::IR::Instruction::Opcode::Or:
		out << " or";
		break;
	case compiler::IR::Instruction::Opcode::Xor:
		out << " xor";
		break;
	default:
		break;
	}
	Operands[0].get()->getType()->Print(out);
	out << " " << Operands[0].get()->getName() << ",";
	Operands[0].get()->getType()->Print(out);
	out << " " << Operands[0].get()->getName();
	out << "        ; <";
	Ty->Print(out);
	out << " >\n";
}
//===---------------------------------------------------------------------===//
// StoreInst Implementation
StoreInst::StoreInst(ValPtr Val, ValPtr Ptr, BBPtr InsertAtEnd) :
		Instruction(Type::getVoidType(), Opcode::Store)
{
	Operands.resize(2);
	Operands.push_back(Use(Val, this));
	Operands.push_back(Use(Ptr, this));
	assert(Val && Ptr && "Both operands must be non-null!");
	// e.g. %retval = alloca i32
	//      store i32 10, i32* %retval
	// %retval must have i32* type.
	assert(Ptr->getType()->isPointerTy() && "Ptr must have pointer type!");
	assert(Val->getType().get() == 
			std::dynamic_pointer_cast<PointerType>(Ptr->getType())->getElementTy().get()
			&& "Ptr must be a pointer to Val type!");
}

StoreInstPtr StoreInst::Create(ValPtr Val, ValPtr Ptr)
{
	return std::make_shared<StoreInst>(Val, Ptr);
}

/// \brief Print the StoreInst info.
/// e.g.	return parm - 1;
///			%add = add i32 %tmp, 1
///			store i32 %add, i32* %retval
void StoreInst::Print(std::ostringstream& out)
{
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
AllocaInst::AllocaInst(TyPtr Ty, ValPtr Val, std::string Name, BBPtr InsertAtEnd) :
	UnaryOperator(PointerType::get(Ty), Opcode::Alloca, nullptr, nullptr), 
	AllocatedType(Ty)
{}

AllocaInst::~AllocaInst() {}

AllocaInstPtr AllocaInst::Create(TyPtr Ty)
{
	return std::make_shared<AllocaInst>(Ty);
}

/// \brief Print the AllocaInst info.
/// e.g.	func add() -> int
///			
///			%retval = alloca i32
void AllocaInst::Print(std::ostringstream& out)
{
	out << Name << " = alloca";
	AllocatedType->Print(out);
	out << "        ; <";
	Ty->Print(out);
	out << " >\n";
}

//===---------------------------------------------------------------------===//
// Implements the Terminatror inst.
TerminatorInst::~TerminatorInst() {}

//===---------------------------------------------------------------------===//
// Implements CallInst.
CallInst::CallInst(FuncTypePtr FTy, ValPtr Func, std::vector<ValPtr> Args,  std::string Name, 
				BBPtr InsertAtEnd) : 
		Instruction(FTy->getReturnType(), Opcode::Call), FTy(FTy)
{
	Operands.resize(1 + Args.size());
	Operands.push_back(Use(Func, this));
	assert(Args.size() == FTy->getNumParams() && "The number of parameters is different!");
	unsigned ArgsNum = Args.size();
	for (unsigned i = 0; i < ArgsNum; i++)
	{
		assert((*FTy)[i].get() == Args[i]->getType().get() && "The parameter type is different! ");
		Operands.push_back(Use(Args[i], this));
	}
}

CallInst::~CallInst() {}

CallInstPtr CallInst::Create(ValPtr Func, std::vector<ValPtr> Args, 
			std::string Name, InstPtr InsertBefore)
{
	auto func = std::dynamic_pointer_cast<Function>(Func);
	assert(func && "The Value must be the 'Function'!");
	auto functy = std::dynamic_pointer_cast<FunctionType>(func->getFunctionType());
	assert(functy && "The function must have 'FunctionType'");
	return std::make_shared<CallInst>(functy, Func, Args);
}

ValPtr CallInst::getArgOperand(unsigned i) const
{
	assert(i < FTy->getNumParams() && "Index out of range!");
	return Operands[i + 1].get();
}

void CallInst::setArgOperand(unsigned i, ValPtr v)
{
	assert(i < FTy->getNumParams() && "Index out of range!");
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
void CallInst::Print(std::ostringstream& out)
{
	if (!Ty->isVoidType())
	{
		out << "%" << Name << " =";
	}
	out << " call";
	Ty->Print(out);
	out << " " << Name << "(";
	if (Operands.size() > 1)
	{
		// Print the parameter information.
		unsigned Length = Operands.size();
		for (unsigned i = 1; i < Length; i++)
		{
			Operands[i].get()->getType()->Print(out);
			out << " " << Operands[i].get()->getName();
			if (i != Length - 1) 
			{ 
				out << ","; 
			}
		}
	}
	out << ")" << "        ; <";
	Ty->Print(out);
	out << ">";
}
//===---------------------------------------------------------------------===//
// Implements ReturnInst.
ReturnInst::ReturnInst(ValPtr retVal, BBPtr InsertAtEnd) : TerminatorInst(Opcode::Ret)
{
	Operands.resize(1);
	Operands.push_back(Use(retVal, this));
}

ReturnInstPtr ReturnInst::Create(ValPtr retVal, BBPtr InsertAtEnd)
{
	return std::make_shared<ReturnInst>(retVal, InsertAtEnd);
}

ReturnInst::~ReturnInst() {}

// Out-of-line ReturnInst method, put here so the c++ compiler can 
// choose to emit the vtable for the class in this translation unit :).
void ReturnInst::setSuccessor(unsigned idx, BBPtr B)
{
	assert(0 && "has no successors!");
}

/// \brief Print the ReturnInst info.
///	e.g.	return num;
///			ret i32 %0;
///				or
///			return;
///			ret void;
void ReturnInst::Print(std::ostringstream& out)
{
	out << "ret";
	Ty->Print(out);
	out << " " << Name;
}
//===---------------------------------------------------------------------===//
// Implements the BranchInst class.
BranchInst::BranchInst(BBPtr IfTrue, BBPtr InsertAtEnd) : TerminatorInst(Opcode::Br)
{
	assert(IfTrue && "Dest basic block may not be null!");
	Operands.resize(1);
	Operands.push_back(Use(IfTrue, this));
}

BranchInst::BranchInst(BBPtr IfTrue, BBPtr IfFalse, ValPtr CondV, BBPtr InsertAtEnd) : 
		TerminatorInst(Opcode::Br)
{
	assert(IfTrue && IfFalse && "Dest basic blocks may not be null!");
	assert(CondV->getType()->isBoolTy() && "May only branch on boolean predicates!");
	Operands.resize(3);
	Operands.push_back(Use(IfTrue, this));
	Operands.push_back(Use(IfFalse, this));
	Operands.push_back(Use(CondV, this));
}

BranchInst::~BranchInst() {}

BrInstPtr BranchInst::Create(BBPtr IfTrue, BBPtr InsertAtEnd)
{
	return std::make_shared<BranchInst>(IfTrue, InsertAtEnd);
}

BrInstPtr BranchInst::Create(BBPtr IfTrue, BBPtr IfFalse, ValPtr CondV, BBPtr InsertAtEnd)
{
	return std::make_shared<BranchInst>(IfTrue, IfFalse, CondV, InsertAtEnd);
}

ValPtr BranchInst::getSuccessor(unsigned i) const
{
	assert((Operands.size() == 1 && i == 0) || 
		(Operands.size() == 3 && (i == 0 || i == 1)) && "Get successors error!");
	return Operands[i].get();
}

void BranchInst::setSuccessor(unsigned idx, BBPtr NewSucc)
{
	assert((idx == 0 || idx == 1) && "Index out of range!");
	Operands[idx] = NewSucc;
}

/// \brief Print the BranchInst info.
/// e.g.	if (num > parm)
///			
///			br i1 %cmp, label %if.then, label %if.end
void BranchInst::Print(std::ostringstream& out)
{
	out << "br bool " 
		<< Operands[0].get()->getName() 
		<< ", label " 
		<< Operands[0].get()->getName();
}
//===---------------------------------------------------------------------===//
// Implements the Load instruciton.
LoadInst::LoadInst(ValPtr Addr, std::string Name, BBPtr InsertAtEnd) : 
		UnaryOperator(nullptr, Opcode::Load, Addr)
{
	auto PointerTy = std::dynamic_pointer_cast<PointerType>(Addr->getType());
	assert(PointerTy && "Addr must have pointer type.");
	Ty = PointerTy->getElementTy();
}

LoadInst::~LoadInst() {}

LoadInstPtr LoadInst::Create(ValPtr Addr)
{
	return std::make_shared<LoadInst>(Addr);
}

/// \brief Print the LoadInst info.
///	e.g.	int num = parm + 10£»
///
///			%tmp = load i32* %parm.addr
void LoadInst::Print(std::ostringstream& out)
{
	out << Name << " = load";
	Operands[0].get()->getType()->Print(out);
	out << " " << Operands[0].get()->getName();
	out << "        ; <";
	Ty->Print(out);
	out << " >\n";
}