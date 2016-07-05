//===-------------------------------CGStmt.cpp----------------------------===//
//
// This contains code to emit Stmt nodes as LLVM code.
//
//===---------------------------------------------------------------------===//
#include "../../include/IRBuild/IRBuilder.h"
using namespace compiler::ast;
using namespace compiler::IR;
using namespace compiler::IRBuild;

ValPtr ModuleBuilder::visit(const IfStatement* ifstmt)
{	
	EmitIfStmt(ifstmt);
	return nullptr;
}

/// EmitBlock - Emit the given block \arg BB and set it as the insert point.
void ModuleBuilder::EmitBlock(BBPtr BB, bool IsFinished)
{
	EmitBrach(BB);
	if (IsFinished && BB->use_empty())
	{
		BB.reset();
		return;
	}
	CurFunc->CurFn->getBasicBlockList().push_back(BB);
	SetInsertPoint(BB);
}

/// \brief Emit a branch from the current block to the target one if this
/// was a real block.
void ModuleBuilder::EmitBrach(BBPtr Target)
{
	if (!CurBB || CurBB->getTerminator())
	{
		// If there is no insert point or the previous block is already
		// terminated, don't touch it.
	}
	else
	{
		// Otherwise, create a fall-through branch.
		CreateBr(Target);
	}
	ClearInsertionPoint();
}

void ModuleBuilder::EmitReturnBlock()
{
	// For cleanliness, we try to avoid emitting the return block for simple cases.
	
	EmitBlock(CurFunc->ReturnBlock);
}

/// \brief EmitWhileStmt - Emit the code for while statement.
/// e.g.	while (num < lhs)	-------------------------  ---> Pre-Block
///			{					|		...				|
///				lhs += 2;		| br label %while.cond	|
///			}					-------------------------
///									
///								------------------------------------------------- ---> while.cond
///								| %tmp = load i32* %num							|
///								| %tmp1 = load i32* %lhs						|
///								| %cmp = cmp lt i32 %tmp, %tmp1					|
///								| br %cmp, label %while.body, label %while.end	|
///								-------------------------------------------------
///			
///								----------------------------- ---> while.body
///								| %tmp2 = load i32* %lhs	|
///								| %add = add i32 %tmp2, 2	|
///								| store i32 %add, i32* %lhs	|
///								| br label %while.cond		|
///								-----------------------------
///							
///								----------------------------- ---> while.end
///								|			...				|
///								-----------------------------
void ModuleBuilder::EmitWhileStmt(const WhileStatement* whilestmt)
{

}

ValPtr ModuleBuilder::visit(const WhileStatement* whilestmt)
{
	EmitWhileStmt(whilestmt);
	return nullptr;
}

/// \brief Dispatched the task to the children.
ValPtr ModuleBuilder::visit(const CompoundStmt* comstmt)
{
	// (1) Switch the scope.
	// e.g.	var num = 10;
	//	func add(lhs:int, rhs : int) -> int			------------ <---- Symbol table for the 'add'
	//	{						----> Old scope.   |	 inc    |
	//		var inc = 100;							------------ <---- Not visited yet.
	//		if (lhs > rhs)						   | AnonyScope |
	//		{					----> New scope.	------------ <---- Not visited yet.
	//			lhs += 10;						   | AnonyScope	|
	//		}										------------
	//		else
	//		{
	//			rhs += 10;
	//		}
	//		return lhs + rhs + inc;
	//	}
	// Note: Search the first not accessed ScopeSymbol.
	auto symTab = CurScope->getSymbolTable();
	auto num = symTab.size();
	for (unsigned i = 0; i < num; i++)
	{
		if (std::shared_ptr<ScopeSymbol> scope = std::dynamic_pointer_cast<ScopeSymbol>(symTab[i]))
		{
			if (scope->isVisitedForIRGen())
				continue;
			CurScope = scope->getScope();
		}
		continue;
	}

	// (2) Generated the code for children.
	unsigned size = comstmt->getSize();
	for (unsigned i = 0; i < size; i++)
	{
		(*comstmt)[i]->Accept(this);
	}

	// (3) Switch the scope back.
	CurScope = CurScope->getParent();

	return nullptr;
}

ValPtr ModuleBuilder::visit(const ReturnStatement* retstmt)
{
	return nullptr;
}

void ModuleBuilder::EmitIfStmt(const IfStatement* ifstmt)
{
	// If the condition constant folds and can be elided, try to avoid emitting
	// the condition and the dead arm of the if/else
	bool CondConstant;

	/// If the condition expr can be evaluated, true or false.
	if (evaluator.EvaluateAsBooleanCondition(ifstmt->getCondition(), CondConstant))
	{
		StmtASTPtr Executed = ifstmt->getThen();
		StmtASTPtr Skipped = ifstmt->getElse();
		/// Condition expression can be evaluated to the false value.
		if (!CondConstant)
			std::swap(Executed, Skipped);

		/// C/C++ has goto statement, so there is one situation that we can't elide the specified block.
		/// e.g		if (10 != 10)
		///			{					-----> Evaluate the condition expression to be false, dead 'then'.
		///		RET:	return num;		-----> The 'RET' label means that there is possible that 
		///										'return num' can be execeted.
		///			}
		///			else
		///			{
		///				...
		///			}
		/// But moses have no goto statements, so we don't need to worry.
		if (Executed)
		{
			Executed->Accept(this);
		}
	}

	// othewise, the condition did not fold, or we couldn't elide it. Just emit
	// the condition branch.

	// Create blocks for then and else cases. Insert the 'then' block at the end
	// of the function.
	auto ThenBlock = BasicBlock::Create("if.then", nullptr);
	auto ContBlock = BasicBlock::Create("if.end", nullptr);
	auto ElseBlock = ContBlock;
	if (ifstmt->getElse())
		ElseBlock = BasicBlock::Create("if.else", nullptr);

	// Once the blocks are created, we can emit the conditional branch that choose between
	// them. 
	EmitBranchOnBoolExpr(ifstmt->getCondition(), ThenBlock, ElseBlock);

	// Emit the 'then' code.
	EmitBlock(ThenBlock);
	ifstmt->Accept(this);
	EmitBrach(ContBlock);

	// Emit the 'else' code if present.
	if (auto Else = ifstmt->getElse())
	{
		EmitBlock(ElseBlock);
		Else.get()->Accept(this);
		EmitBrach(ContBlock);
	}

	// Emit the continuation block for code after the if.
	EmitBlock(ContBlock, true);
}


void ModuleBuilder::EmitFunctionBody(StmtASTPtr body)
{
	EmitCompoundStmt(body.get());
}

ValPtr ModuleBuilder::EmitCompoundStmt(const StatementAST* stmt)
{
	stmt->Accept(this);
	return nullptr;
}