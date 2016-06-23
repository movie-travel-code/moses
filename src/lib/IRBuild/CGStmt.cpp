//===-------------------------------CGStmt.cpp----------------------------===//
//
// This contains code to emit Stmt nodes as LLVM code.
//
//===---------------------------------------------------------------------===//
#include "../../include/IRBuild/IRBuilder.h"
using namespace compiler::ast;
using namespace compiler::IR;
using namespace compiler::IRBuild;

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

/// \brief Emit a branch from the current block to the target one if this
/// was a real block.
/// ���磺
///		func add(lhs : int, rhs : int) -> int
///		{
///			int num = 0;
///			num = lhs + rhs;
///			if (num == 0)
///			{
///				return num + 1;
///			}
///			return num + 1;
///		}
///		----------------------------
///		|	%1 = alloca i32			|
///		|	%2 = alloca i32			|
///		|	%3 = alloca i32			|
///		|	%num = alloca i32		|
///		|	store i32 %rhs, i32* %2	|
///		|	store i32 %lhs, i32* %3	|
///		|	%4 = load i32* %3		|
///		|	%5 = load i32* %2		|
///		|	%6 = add i32 %4, %5		|
///		|	store i32 %6, i32* %num	|
///		|	%7 = load i32* %num		|
///		|	%8 = cmp eq i32 %7, 0	|
///		|	br i1 %8, label %9, label %12 |
///		-----------------------------	
/// 
///		; <label>: 9
///			%10 = load i32* %num
///			%11 = add i32 %10, 1
///			store i32 %11, i32* %1
///			br label %14
///		
///		; <label>:12
///			%13 = load i32* %num
///			store i32 %13, i32* %1
///			br label %14
///
///		; <label>:14
///			%15 = load i32* %1
///			ret 32 %15	
void ModuleBuilder::EmitBrach(BBPtr Target)
{

}

void ModuleBuilder::visit(const WhileStatement* whilestmt)
{
	// Cond->CodeGen()

	// �м�Ҫ����Щ��תָ��

	// ���м����ɵ���BasicBlock

	// CompoumdStmt->CodeGen()
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