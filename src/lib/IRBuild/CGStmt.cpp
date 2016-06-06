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
	/// \brief ��If����condition expr����evaluate�����Ƿ��ƶϳ�Ϊconstant.
	ConstantEvaluator evaluator;
	bool CondCanBeFold = evaluator.EvaluateAsBooleanCondition(ifstmt->getCondition(), CondConstant);

	/// ConditionExpr��Ϊ�棬ɾ��else��֧������еĻ���
	if (CondCanBeFold)
	{
		StmtASTPtr Executed = ifstmt->getThen();
		StmtASTPtr Skipped = ifstmt->getElse();
		/// ConditionExpr��Ϊ�٣���ɾ��true��֧
		/// �����û��else��֧�Ļ���ɾ������IfStmt�����ɣ�
		if (!CondConstant)
			std::swap(Executed, Skipped);

		/// ע����C/C++�д���һ�������Ҫע�⣬����ʡ�Ե�block���п�����goto��Ŀ��
		/// label������Clang��Ҫ���block���Ƿ���label��
		/// ����mosesû��goto��Ҳ����˵�����ܴ������������
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

	// �м�Ҫ����Щ��תָ��
	// ����һЩ�򵥵��Ż��жϣ����condition expr�Ƕ�ֵ����ɾ���ٷ�֧���ִ���

	// Then->CodeGen()

	// Else->CodeGen()
}

void ModuleBuilder::visit(const WhileStatement* whilestmt)
{
	// Cond->CodeGen()

	// �м�Ҫ����Щ��תָ��

	// ���м����ɵ���BasicBlock

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