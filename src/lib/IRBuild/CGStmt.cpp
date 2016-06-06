//===-------------------------------CGStmt.cpp----------------------------===//
//
// This contains code to emit Stmt nodes as LLVM code.
//
//===---------------------------------------------------------------------===//
#include "../../include/IRBuild/IRBuilder.h"
using namespace compiler::ast;
using namespace compiler::IR;

void ModuleBuilder::visit(const IfStatement* ifstmt)
{
	// If the condition constant folds and can be elided, try to avoid emitting
	// the condition and the dead arm of the if/else
	bool CondConstant;
	/// \brief 对If语句的condition expr进行evaluate，看是否推断成为constant.
	ConstantEvaluator evaluator;
	bool CondCanBeFold = evaluator.EvaluateAsBooleanCondition(ifstmt->getCondition(), CondConstant);

	/// ConditionExpr恒为真，删除else分支（如果有的话）
	if (CondCanBeFold)
	{
		StmtASTPtr Executed = ifstmt->getThen();
		StmtASTPtr Skipped = ifstmt->getElse();
		/// ConditionExpr恒为假，则删除true分支
		/// （如果没有else分支的话，删除整个IfStmt的生成）
		if (!CondConstant)
			std::swap(Executed, Skipped);

		/// 注意在C/C++中存在一种情况需要注意，就是省略的block中有可能有goto的目标
		/// label，所以Clang需要检查block中是否有label。
		/// 但是moses没有goto，也就是说不可能存在上述情况。
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

	// 中间要穿插些跳转指令
	// 进行一些简单的优化判断，如果condition expr是定值，则删除假分支部分代码

	// Then->CodeGen()

	// Else->CodeGen()
}

void ModuleBuilder::visit(const WhileStatement* whilestmt)
{
	// Cond->CodeGen()

	// 中间要穿插些跳转指令

	// 这中间生成的是BasicBlock

	// CompoumdStmt->CodeGen()
}

void ModuleBuilder::visit(const DeclStatement* declstmt)
{

}

void ModuleBuilder::visit(const CompoundStmt* comstmt)
{
	unsigned size = comstmt->getSize();
	for (int i = 0; i < size; i++)
	{
		(*comstmt)[i]->Accept(this);
	}
}

void ModuleBuilder::visit(const ExprStatement* exprstmt)
{
}