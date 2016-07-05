//===--------------------------CGExpr.cpp---------------------------------===//
//
// This contains code to emit Expr nodes.
//
//===---------------------------------------------------------------------===//
#include "../../include/IRBuild/IRBuilder.h"
using namespace compiler::IR;
using namespace compiler::IRBuild;

/// \brief Emit branch condition.
void ModuleBuilder::EmitBranchOnBoolExpr(ExprASTPtr Cond, BBPtr TrueBlock, BBPtr FalseBlock)
{
	if (BinaryPtr CondBOp = std::dynamic_pointer_cast<BinaryExpr>(Cond))
	{
		// Handle X && Y in a conditon.
		if (CondBOp->getOpcode() == "&&")
		{
			// If we have "true && X", simplify the code. "0 && X" would have constant
			// folded if the case was simple enough.
			bool result;
			// Here the results of the evaluation can only be ture value.
			if (evaluator.EvaluateAsBooleanCondition(CondBOp->getLHS(), result))
				// br(true && X) -> br(X).
				return EmitBranchOnBoolExpr(CondBOp->getRHS(), TrueBlock, FalseBlock);

			// br(X && true) -> br(X)
			if (evaluator.EvaluateAsBooleanCondition(CondBOp->getRHS(), result))
				// br(X && true) -> br (X)
				return EmitBranchOnBoolExpr(CondBOp->getLHS(), TrueBlock, FalseBlock);

			// Emit the LHS as a conditional. If the LHS conditional is false, we want to jump
			// to the FalseBlock. 
			// Short-circuit evaluation https://en.wikipedia.org/wiki/Short-circuit_evaluation
			//			extern int num;
			// e.g.		if (lhs > rhs && num == 0)	----> We can't evaluate the "lhs > rhs" on the fly.
			//			{							----> if.then block
			//				...
			//			}
			//			else
			//			{							----> if.else block
			//				...
			//			}
			// At first, we generate code for "lhs > rhs", connecting "num == 0" for true and 
			// "if.else" for false.
			BBPtr LHSTrue = CreateBasicBlock("and.lhs.true");
			EmitBranchOnBoolExpr(CondBOp->getLHS(), LHSTrue, FalseBlock);
			EmitBlock(LHSTrue);

			EmitBranchOnBoolExpr(CondBOp->getRHS(), TrueBlock, FalseBlock);
			return;
		}
		else if (CondBOp->getOpcode() == "||")
		{
			// If we have "false || X", simplify the code. "true || X" would have constant folded
			// if the case was simple enough.
			bool result;
			if (evaluator.EvaluateAsBooleanCondition(CondBOp->getLHS(), result))
				// br(false || X) -> br(X)
				return EmitBranchOnBoolExpr(CondBOp->getRHS(), TrueBlock, FalseBlock);
			if (evaluator.EvaluateAsBooleanCondition(CondBOp->getRHS(), result))
				// br(X || false) -> br(X)
				return EmitBranchOnBoolExpr(CondBOp->getLHS(), TrueBlock, FalseBlock);

			// Refer to "And", here also implement the short-circuit principle.
			// Emit the LHS as a conditional. If the LHS conditional is true, we want to jump
			// to the TrueBlock.
			BBPtr LHSFalse = CreateBasicBlock("or.lhs.false");
			EmitBranchOnBoolExpr(CondBOp->getLHS(), TrueBlock, LHSFalse);
			EmitBlock(LHSFalse);

			EmitBranchOnBoolExpr(CondBOp->getRHS(), TrueBlock, FalseBlock);
			return;
		}
	}

	// Deal with the situation of br(!X).
	if (UnaryPtr CondUOp = std::dynamic_pointer_cast<UnaryExpr>(Cond))
	{
		if (CondUOp->getOpcode() == "!")
			return EmitBranchOnBoolExpr(CondUOp->getSubExpr(), FalseBlock, TrueBlock);
	}

	// Emit the code with the fully general case.
	ValPtr CondV = Cond->Accept(this);
	CreateCondBr(CondV, TrueBlock, FalseBlock);
}

ValPtr ModuleBuilder::EvaluateExprAsBool(ExprASTPtr E)
{
	return 0;
}

/// \brief Handle the algebraic and boolean operation, include '+' '-' '*' '/' '%'
///	'<' '>' '<=' '>=' '==' '!='.
ValPtr ModuleBuilder::EmitAlgAndBooleanOp(const CGExpr::BinOpInfo& BInfo)
{
	std::string Opcode = BInfo.BE->getOpcode();

	if (Opcode == "+")
	{
		return CreateAdd(BInfo.LHS, BInfo.RHS);
	}
	if (Opcode == "-")
	{
		return CreateSub(BInfo.LHS, BInfo.RHS);
	}
	if (Opcode == "*")
	{
		return CreateMul(BInfo.LHS, BInfo.RHS);
	}
	if (Opcode == "/")
	{
		return CreateDiv(BInfo.LHS, BInfo.RHS);
	}
	if (Opcode == "%")
	{
		return CreateRem(BInfo.LHS, BInfo.RHS);
	}
	if (Opcode == "<")
	{
		return CreateCmpLT(BInfo.LHS, BInfo.RHS);
	}
	if (Opcode == ">")
	{
		return CreateCmpGT(BInfo.LHS, BInfo.RHS);
	}
	if (Opcode == "<=")
	{
		return CreateCmpLE(BInfo.LHS, BInfo.RHS);
	}
	if (Opcode == ">=")
	{
		return CreateCmpGE(BInfo.LHS, BInfo.RHS);
	}
	if (Opcode == "==")
	{
		return CreateCmpEQ(BInfo.LHS, BInfo.RHS);
	}
	if (Opcode == "!=")
	{
		return CreateCmpNE(BInfo.LHS, BInfo.RHS);
	}
	assert(0 && "Unreachable program point.");
	// An invalid LValue, the assert will ensure that this point is never reached.
	return nullptr;
}

/// \brief Handle the assign operation, include '='.
/// e.g.	mem = num;		---->	-----------------------------
///									| %tmp = load i32* %num		|
///									| store i32 %tmp, i32* %mem	|
///									-----------------------------
ValPtr ModuleBuilder::EmitBinAssignOp(const BinaryExpr* B)
{
	ValPtr RHSV = B->getRHS()->Accept(this);
	LValue LHSAddr = EmitLValue(B->getLHS().get());

	// Store the value into the LHS.
	// --------------------Assignment operators-------------------------
	// An assignment operator stores a value in the object designated by the left operand. 
	// An assignment expression has the value of the left operand after the assignment,
	// but is not an lvalue.
	// http://stackoverflow.com/questions/22616044/assignment-operator-sequencing-in-c11-expressions
	//---------------------Assignment operators-------------------------
	EmitStoreThroughLValue(RValue::get(RHSV), LHSAddr);
	return EmitLoadOfLValue(LHSAddr, B->getType()).getScalarVal();
}

/// \brief Handle the compound assign operation, include '*=' '/=' '%=' '+=' '-='
///	'&&=' '||='.
///	e.g.		mem += num;		---->	-----------------------------
///										| %tmp = load i32* %num		|
///										| %tmp1 = load i32* %mme	|
///										| %add = i32 %tmp, i32 %tmp1|						|
///										| store i32 %add, i32* %mem	|
///										-------------------------
ValPtr ModuleBuilder::EmitCompoundAssignOp(const BinaryExpr* BE)
{
	// (1) Emit the RHS first.
	ValPtr RHSV = BE->getRHS()->Accept(this);

	// (2) Load the LHS.
	LValue LHSLV = EmitLValue(BE->getLHS().get());
	ValPtr LHSV = EmitLoadOfLValue(LHSLV, BE->getLHS()->getType()).getScalarVal();

	// (3) Perform the operation.
	CGExpr::BinOpInfo info;
	info.LHS = LHSV;
	info.RHS = LHSV;
	info.BE = BE;
	info.Ty = BE->getType();

	ValPtr ResultV = EmitAlgAndBooleanOp(info);

	// (4) Store the result value into the LHS lvalue.
	// Note: 'An assignment expression has the value of the left operand after the
	//		assignment...'
	// To Do: As a result of copying the Clang, 'EmitStoreThroughLValue()' and 
	// 'EmitLoadOfLValue()' have a little difference(one need AST type and the other
	// not).
	EmitStoreThroughLValue(RValue::get(ResultV), LHSLV);

	return EmitLoadOfLValue(LHSLV, BE->getLHS()->getType()).getScalarVal();
}

/// \brief Handle the binary expression.
ValPtr ModuleBuilder::EmitBinaryExpr(const BinaryExpr* B)
{
	std::string Opcode = B->getOpcode();
	CGExpr::BinOpInfo info;
	info.LHS = B->getLHS()->Accept(this);
	info.RHS = B->getRHS()->Accept(this);
	info.Ty = B->getType();
	info.BE = B;
	if (Opcode == "+" || Opcode == "-" || Opcode == "*" || Opcode == "/" ||
		Opcode == "%" || Opcode == "<" || Opcode == ">" || Opcode == "<=" ||
		Opcode == ">=" || Opcode == "==" || Opcode == "!=")
		return EmitAlgAndBooleanOp(info);

	if (Opcode == "=")
		return EmitBinAssignOp(B);

	if (Opcode == "*=" || Opcode == "/=" || Opcode == "%=" || Opcode == "+=" || 
		Opcode == "-=" || Opcode == "&&=" || Opcode == "||=")
		return EmitCompoundAssignOp(B);
	assert(0 && "Unreachable program point.");
	return nullptr;
}

/// EmitLValue - Emit code to compute a designator that specifies the location of 
/// the expression. For example, 'a = 10' or 'a.start.hieght = 10', we need compute
/// a location to store '10'.
LValue ModuleBuilder::EmitLValue(const Expr* E)
{
	if (const DeclRefExpr* DRE = dynamic_cast<const DeclRefExpr*>(E))
	{
		EmitDeclRefLValue(DRE);
	}
	// Is not yet implemented.
	if (const MemberExpr* ME = dynamic_cast<const MemberExpr*>(E))
	{}
	assert(0 && "Unreachable program point.");
	return LValue();
}

/// \brief Get the decl address(Alloca Inst).
LValue ModuleBuilder::EmitDeclRefLValue(const DeclRefExpr* DRE)
{
	// DeclRefExpr is still very simple. --- 2016-7-5
	std::string DeclName = DRE->getDeclName();
	auto Sym = CurScope->Resolve(DeclName);
	assert(Sym && "No alloca instruciton created for the decl?");
	if (std::shared_ptr<VariableSymbol> VarSym = 
		std::dynamic_pointer_cast<VariableSymbol>(Sym))
	{
		auto Alloca = VarSym->getAllocaInst();
		assert(Alloca && "No alloca instruciton created for the VarDecl?");
		return LValue::MakeAddr(Alloca);
	}
	if (std::shared_ptr<ParmDeclSymbol> ParmSym = 
			std::dynamic_pointer_cast<ParmDeclSymbol>(Sym))
	{
		auto Alloca = ParmSym->getAllocaInst();
		assert(Alloca && "No alloca instruciton created for the ParmDecl?");
		return LValue::MakeAddr(Alloca);
	}
	assert(0 && "Unreachable program point.");
	return LValue();
}

/// \brief EmitLoadOfLValue - Given an expression with complex type that represents
/// a l-value, this method emits the address of the l-value, then loads and returns
/// the result.
ValPtr ModuleBuilder::EmitLoadOfLValue(const Expr* E)
{
	return EmitLoadOfLValue(EmitLValue(E), E->getType()).getScalarVal();
}

/// \brief EmitLoadOfLValue - Given an expression that represents a value lvalue,
/// this method emits the address of the lvalue, then loads the result as an rvalue,
/// returning the rvalue.
RValue ModuleBuilder::EmitLoadOfLValue(LValue LV, compiler::IRBuild::ASTTyPtr ExprTy)
{
	ValPtr Ptr = LV.getAddress();
	auto V = CreateLoad(LV.getAddress());
	return RValue::get(V);
}

/// \brief EmitUnaryExpr - Emit code for unary expression.
ValPtr ModuleBuilder::EmitUnaryExpr(const UnaryExpr* UE)
{
	std::string Opcode = UE->getOpcode();
	// Minus
	if (Opcode == "-")
	{
		ValPtr OperandV = UE->getSubExpr()->Accept(this);
		return CreateNeg(OperandV, "neg");
	}

	// We must distinguish pre between post.
	if (Opcode == "++")
	{
		if (UE->isLValue())
			EmitPrePostIncDec(UE, true, true);
		else
			EmitPrePostIncDec(UE, true, false);
	}
	if (Opcode == "--")
	{
		if (UE->isLValue())
			EmitPrePostIncDec(UE, false, true);
		else
			EmitPrePostIncDec(UE, false, false);
	}

	if (Opcode == "!")
	{
		ValPtr OperandV = UE->getSubExpr()->Accept(this);
		return CreateNot(OperandV, "not");
	}
}

/// \brief EmitPrePostIncDec - Generate code for inc and dec.
/// e.g.	++num(pre-inc)	------>		-----------------------------
///										| %tmp = load i32* %num		|
///										| %inc = add i32 %tmp, 1	|	----> the result
///										| store i32 %inc, i32* %num |
///										-----------------------------
///			num++(post-inc) ------>		-----------------------------
///										| %tmp1 = load i32* %num	|	----> the result
///										| %inc2 = add i32 %tmp, 1	|
///										| store i32 %inc2, i32* %num|
///										-----------------------------
/// Note: Since only integer can be increase and the decrese.
ValPtr ModuleBuilder::EmitPrePostIncDec(const UnaryExpr* UE, bool isInc, bool isPre)
{
	int AmoutVal = isInc ? 1 : -1;
	ValPtr NextVal;
	// (1) Get the sub expression's address.
	LValue LV = EmitLValue(UE->getSubExpr().get());
	compiler::IRBuild::ASTTyPtr ValTy = UE->getSubExpr()->getType();

	// (2) Get the sub expression's value.
	ValPtr InVal = EmitLoadOfLValue(LV, ValTy).getScalarVal();

	// (3) Perform operation.
	NextVal = ConstantInt::get(AmoutVal);
	NextVal = CreateAdd(InVal, NextVal, isInc ? "inc" : "dec");
}

ValPtr ModuleBuilder::visit(const BinaryExpr* B)
{
	return EmitBinaryExpr(B);
}

ValPtr ModuleBuilder::visit(const DeclRefExpr* DRE)
{
	return EmitLoadOfLValue(DRE);
}

ValPtr ModuleBuilder::visit(const BoolLiteral* BL)
{
	if (BL->getVal())
		return ConstantBool::getTrue();
	else
		return ConstantBool::getFalse();
}

ValPtr ModuleBuilder::visit(const NumberExpr* NE)
{
	return ConstantInt::get(NE->getVal());
}

/// \brief Generate the code for UnaryExpr, include ''
/// Note: Post-increment and Pre-increment can be distinguished by ExprValueKind.
//// ++x is lvalue and x++ is rvalue.
ValPtr ModuleBuilder::visit(const UnaryExpr* UE)
{	
	return EmitUnaryExpr(UE);
}

ValPtr ModuleBuilder::visit(const MemberExpr* ME)
{
	return nullptr;
}

/// Emit an expression as an initializer for a variable at the given location. 
///
/// \param init - the initializing expression.
/// \param var - the variable to act if we're initializing.
/// \param lvalue - the location.
///
/// For example:
///		int mem = 10;
///		int num = add(mem) + mem * 10;
///	We need to handle the "func() + mem * 10" code generation.
/// First, we should alloca space for 'num'.
///	----	%num = alloca i32
///	Second, walk the AST of 'add(mem) + mem * 10' to generator IR.
/// ----	(1) %2 = load i32* %mem.
/// ----	(2) %3 = call i32 add(i32 %2)
/// ----	(3) %4 = load i32* %mem.
/// ----	(4) %5 = mul i32 %4, 10
/// ----	(5) %6 = add i32 %3, %5
///	----	(6) store i32 %6, i32* %num
/// Note: As shown above, we need the '%num' to initialize the value.
void ModuleBuilder::EmitExprAsInit(const Expr* init, const VarDecl* D, LValue lvalue)
{

}

/// EmitScalarExpr - Emit the computation of the specified expression of
/// scalar type, returning the result.
ValPtr ModuleBuilder::EmitScalarExpr(const Expr* E)
{
	return E->Accept(this);
}

/// EmitStoreThroughLValue - Store the specified rvalue into the specified
/// lvalue, where both are guaranteed to the have the same type.
void ModuleBuilder::EmitStoreThroughLValue(RValue Src, LValue Dst, bool isInit)
{
	EmitStoreOfScalar(Src.getScalarVal(), Dst.getAddress());
}

/// \brief Handle the scalar expression.
void ModuleBuilder::EmitStoreOfScalar(ValPtr Value, ValPtr Addr)
{
	CreateStore(Value, Addr);
}