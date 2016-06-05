//===------------------------EvaluatedExprVisitor.h-----------------------===//
//
// This file defines the EvaluateExprVisitor class.
// 
//===---------------------------------------------------------------------===//
#include "../../include/Parser/EvaluatedExprVisitor.h"
using namespace compiler::ast;

//===-----------------------------------------------------------------------------------===//
// EvaluatedExprVisitorBase's Impletation
bool EvaluatedExprVisitorBase::Evaluate(const Expr* expr, EvalInfo& info)
{
	if (const BinaryExpr *B = dynamic_cast<const BinaryExpr*>(expr))
	{
		EvalInfo LhsVal(ValueKind::IntKind, EvaluationMode::EM_ConstantFold);
		EvalInfo RhsVal(ValueKind::IntKind, EvaluationMode::EM_ConstantFold);

		if (!Evaluate(B->getLHS(), LhsVal))
			return false;
		if (!Evaluate(B->getRHS(), RhsVal))
			return false;
		return EvalBinaryExpr(B, info, LhsVal, RhsVal);
	}

	if (const UnaryExpr *U = dynamic_cast<const UnaryExpr*>(expr))
	{
		EvalInfo subVal(ValueKind::IntKind, EvaluationMode::EM_ConstantFold);

		if (!Evaluate(U->getSubExpr(), subVal))
			return false;
		return EvalUnaryExpr(U, info, subVal);
	}

	if (const MemberExpr *M = dynamic_cast<const MemberExpr*>(expr))
	{
		return EvalMemberExpr(M, info);
	}

	if (const DeclRefExpr *DRE = dynamic_cast<const DeclRefExpr*>(expr))
	{
		return EvalDeclRefExpr(DRE, info);
	}

	if (const CallExpr* CE = dynamic_cast<const CallExpr*>(expr))
	{
		return EvalCallExpr(CE, info);
	}

	if (const BoolLiteral* BL = dynamic_cast<const BoolLiteral*>(expr))
	{
		info.evalstatus.kind = ValueKind::BoolKind;
		info.evalstatus.boolVal = BL->getVal();
		return true;
	}

	if (const NumberExpr* Num = dynamic_cast<const NumberExpr*>(expr))
	{
		info.evalstatus.kind = ValueKind::IntKind;
		info.evalstatus.intVal = Num->getVal();
		return true;
	}
	return false;
}

void EvaluatedExprVisitorBase::handleEvalCallTail()
{
	std::pair<int, unsigned> bookeepingInfo = ActiveBookingInfo[ActiveBookingInfo.size() - 1];
	ActiveBookingInfo.pop_back();
	for (int i = 0; i < bookeepingInfo.second; i++)
	{
		ActiveStack.pop_back();
	}
}

/// \brief �ж�FunctionDecl�ܷ�evaluate��������Ƿ񳬳�eval call�Ĳ�������ǰһ�㣩��
const StatementAST* EvaluatedExprVisitorBase::handleEvalCallStart(const CallExpr* CE)
{
	const StatementAST* returnStmt = CE->getFuncDecl()->isEvalCandiateAndGetReturnStmt();

	if (ActiveBookingInfo.size() >= 1)
	{
		return nullptr;
	}
	return returnStmt;
}

/// \brief �����CallExpr()����evaluation.
/// ���磺
/// func add(parm1 : int, parm2 : int) -> int { return parm1 + parm2; }
/// var num = add(10, 20);
/// �������ǻὫ add(10, 20) evaluate��Ϊ30.
/// ��1�� �������ǻ��ʵ��ֵ����eval����¼�� (name, value) pair��Ȼ��Ӧ�õ���������
/// ��2�� ����Ҫ����ֻ����һ��return��䡣����name, value��pairӦ�õ�return����У���������
/// ��3�� ���գ����õ��Ľ��ֵ��ΪCallExpr��evaluate�õ���constant.
bool EvaluatedExprVisitorBase::EvalCallExpr(const CallExpr* CE, EvalInfo &info)
{
	// Note: ����ṹ�����⣬ÿ�����һ��ExprEvaluator��Ҫ�޸Ĵ��룬��Ҫ�޸�.
	ValueKind vk;
	if (typeid(*this).name() == "IntExprEvaluator")
	{
		if (CE->getType()->getKind() != TypeKind::INT)
			return false;
		vk = ValueKind::IntKind;
	}

	if (typeid(*this).name() == "BoolExpeEvaluator")
	{
		if (CE->getType()->getKind() != TypeKind::BOOL)
			return false;
		vk = ValueKind::BoolKind;
	}

	// (0) �����ж�FunctionDecl�ܷ�evaluate������Ƿ񳬳�eval call�Ĳ�������ǰһ�㣩��
	const StatementAST* returnStmt = handleEvalCallStart(CE);

	if (!returnStmt)
	{
		return false;
	}

	// (1) ��ʵ��ֵ����evaluate
	unsigned num = CE->getArgsNum();
	for (int i = 0; i < num; i++)
	{
		const Expr* Arg = CE->getArg(i);

		// ��Arg����evaluate
		EvalInfo info(vk, EvaluationMode::EM_ConstantFold);
		Evaluate(Arg, info);

		// ��� <name, value> pair�洢��������¼��һ���ʵ��info
		ActiveStack.push_back(std::make_pair(CE->getFuncDecl()->getParmName(i), info));
	}

	// ��¼�˴ε�bookkeeping info
	ActiveBookingInfo.push_back(std::make_pair(ActiveBookingInfo.size(), num));

	// (2) ��ȡCallExpr��Ӧ�������е�ReturnStmt��Ȼ��ִ�����㣬�õ�constantֵ��Ȼ�󷵻�.
	EvalInfo returnInfo(vk, EvaluationMode::EM_ConstantFold);
	if (const ReturnStatement* rstmt = dynamic_cast<const ReturnStatement*>(returnStmt))
	{
		const Expr* rexpr = rstmt->getSubExpr();
		Evaluate(rexpr, returnInfo);
	}

	// (3) ��EvalCall������β����
	handleEvalCallTail();

	// (4) �Եõ��Ľ�������ۺϴ���
	info = returnInfo;
	return true;
}

/// \brief ���ڶ�DeclRefExpr����evaluate.
bool EvaluatedExprVisitorBase::EvalDeclRefExpr(const DeclRefExpr* DRE, EvalInfo &info)
{
	// Note: ����ṹ�����⣬ÿ�����һ��ExprEvaluator��Ҫ�޸Ĵ��룬��Ҫ�޸�.
	ValueKind vk;
	if (typeid(*this).name() == "IntExprEvaluator")
	{
		if (DRE->getType()->getKind() != TypeKind::INT)
			return false;
		vk = ValueKind::IntKind;
	}

	if (typeid(*this).name() == "BoolExpeEvaluator")
	{
		if (DRE->getType()->getKind() != TypeKind::BOOL)
			return false;
		vk = ValueKind::BoolKind;
	}

	// ���DeclRefExpr���õı����Ƿ���const������const���ز�����constant-evaluator.
	if (!DRE->getDecl()->isConst())
		return false;
	EvalInfo declInfo(vk, EvaluationMode::EM_ConstantFold);
	if (!Evaluate(DRE->getDecl()->getInitExpr(), declInfo))
	{
		return false;
	}
	info = declInfo;
	return true;
}


//===-----------------------------------------------------------------------------------===//
// IntExprEvaluator's Impletation
bool IntExprEvaluator::EvalBinaryExpr(const BinaryExpr* B, EvalInfo &info,
	EvalInfo &lhs, EvalInfo &rhs)
{
	std::string OpName = B->getOpcode();
	if (OpName == "+")
	{
		info.evalstatus.intVal = lhs.evalstatus.intVal + rhs.evalstatus.intVal;
	}
	else if (OpName == "-")
	{
		info.evalstatus.intVal = lhs.evalstatus.intVal - rhs.evalstatus.intVal;
	}
	else if (OpName == "*")
	{
		info.evalstatus.intVal = lhs.evalstatus.intVal * rhs.evalstatus.intVal;
	}
	else if (OpName == "/")
	{
		info.evalstatus.intVal = lhs.evalstatus.intVal / rhs.evalstatus.intVal;
	}
	else if (OpName == "%")
	{
		info.evalstatus.intVal = lhs.evalstatus.intVal % rhs.evalstatus.intVal;
	}
	else
	{
		return false;
	}
}

/// \brief �������͵ĵ�Ŀ�����������
bool IntExprEvaluator::EvalUnaryExpr(const UnaryExpr* U, EvalInfo &info, EvalInfo &subVal)
{
	std::string OpName = U->getOpcode();
	if (OpName == "-")
	{
		info.evalstatus.intVal = 0 - subVal.evalstatus.intVal;
		return true;
	}
	return false;
}

// To Do: �ȴ�ʵ��
bool IntExprEvaluator::EvalMemberExpr(const MemberExpr* ME, EvalInfo &info)
{
	return false;
	if (const UserDefinedType* UDT = dynamic_cast<UserDefinedType*>(ME->getBase()->getType().get()))
	{
		// UserDefinedType��Member�Ƿ���const���͡�
		if (!UDT->getMemberType(ME->getMemberName())->isConst())
			return false;
		// ��ȡconst��Ա�ĳ�ʼ�����ʽ
		// ���磺
		// class A
		// {
		//		var num : int;
		//		const length : int;
		// }
		// A a;
		// a.num = 10;
		// a.length = 11;	/* const member��Ա��ʼ��*/
		// �����ܹ���ȡ��11�����ʼ�����ʽ���ñ��ʽ����洢��a��Ӧ��Ա�ĳ�ʼ�����ʽ�С�
	}
	return false;
}

//===-----------------------------------------------------------------------------------===//
// BoolExpeEvaluator's Impletation
bool BoolExpeEvaluator::EvalBinaryExpr(const BinaryExpr* B, EvalInfo &info,
	EvalInfo &lhs, EvalInfo &rhs)
{
	std::string OpName = B->getOpcode();
	if (OpName == "||")
	{
		info.evalstatus.boolVal = lhs.evalstatus.boolVal || rhs.evalstatus.boolVal;
		return true;
	}
	else if (OpName == "&&")
	{
		info.evalstatus.boolVal = lhs.evalstatus.boolVal && rhs.evalstatus.boolVal;
		return true;
	}
	else if (OpName == "==")
	{
		/// \brief lhs��bool����(���ڽ��й���������������Ҳ�϶�Ҳ��bool����)
		if (lhs.evalstatus.kind == ValueKind::BoolKind)
		{
			info.evalstatus.boolVal = lhs.evalstatus.boolVal == rhs.evalstatus.boolVal;
		}

		if (lhs.evalstatus.kind == ValueKind::IntKind)
		{
			info.evalstatus.boolVal = lhs.evalstatus.intVal == rhs.evalstatus.intVal;
		}
		return true;
	}
	else if (OpName == "!=")
	{
		/// \brief lhs��bool���ͣ����ڽ��й���������������Ҳ�϶�Ҳ��bool���ͣ�
		if (lhs.evalstatus.kind == ValueKind::BoolKind)
		{
			info.evalstatus.boolVal = lhs.evalstatus.boolVal != rhs.evalstatus.boolVal;
		}

		if (lhs.evalstatus.kind == ValueKind::IntKind)
		{
			info.evalstatus.boolVal = lhs.evalstatus.intVal != rhs.evalstatus.intVal;
		}
	}
	else
	{
		return false;
	}
	return true;
}

/// \brief ����bool������˵����Ŀ�����ֻ��!
bool BoolExpeEvaluator::EvalUnaryExpr(const UnaryExpr* U, EvalInfo &info, EvalInfo &subVal)
{
	info.evalstatus.boolVal = !subVal.evalstatus.boolVal;
	return false;
}

// To Do: ���ڸ��ӣ��۲�ʵ��
bool BoolExpeEvaluator::EvalMemberExpr(const MemberExpr* ME, EvalInfo &info)
{
	return false;
}