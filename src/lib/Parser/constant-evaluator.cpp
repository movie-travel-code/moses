//===------------------------constant-evaluator.cpp-----------------------===//
//
// This file implements the Expr constant evaluator.
//
//===---------------------------------------------------------------------===//
#include "../../include/Parser/constant-evaluator.h"
using namespace compiler::ast;
/// \brief �ú����������Ժ��������Ժ��������Ƶ���
/// �ú���������Ƶ��Ŀ��ٰ汾(FastEvaluateAsRValue)��ǿ���汾(EvaluateAsRValue).
bool ConstantEvaluator::EvaluateAsRValue(const Expr *Exp, EvalInfo &Result) const
{
	// ���ÿ���evaluate�汾������ܹ�����constant-evalute����ֱ�ӷ���
	if (FastEvaluateAsRValue(Exp, Result))
		return true;

	// ����ǿ���汾��Ĭ��evaluate�Ĳ�����32
	if (EvaluateAsRValue(Exp, Result))
		return true;

	return false;
}

/// \brief ���Exp�ܷ��Ƶ���Ϊһ��intֵ�����������ŵ�Result��.
/// ����EvaluateAsRValue(const Expr *Exp, EvalStatus &Result)����.
bool ConstantEvaluator::EvaluateAsInt(const Expr *Exp, int &Result) const
{
	// �ñ��ʽ��������
	if (Exp->getType()->getKind() != TypeKind::INT)
		return false;

	EvalInfo info(ValueKind::IntKind, EvaluationMode::EM_ConstantFold);
	// constant evaluator
	EvaluateAsRValue(Exp, info);

	// �����Ƶ�
	if (info.evalstatus.result == Result::NotConstant)
		return false;
	Result = info.evalstatus.intVal;
	return true;
}

/// \brief ���Exp�ܷ��Ƶ���Ϊһ��boolֵ�����������ŵ�Result��.
/// ����EvaluateAsRValue(const Expr *Exp, EvalStatus &Result)����.
bool ConstantEvaluator::EvaluateAsBooleanCondition(const Expr *Exp, bool &Result) const
{
	// �ñ��ʽ����bool����
	if (Exp->getType()->getKind() != TypeKind::BOOL)
		return false;
	EvalInfo info(EvalStatus::ValueKind::BoolKind, EvaluationMode::EM_ConstantFold);
	// constant evaluator
	EvaluateAsRValue(Exp, info);

	// �����Ƶ�
	if (info.evalstatus.result == EvalStatus::Result::NotConstant)
		return false;
	Result = info.evalstatus.boolVal;
	return true;
}

/// \brief FastEvaluateAsRValue�Ŀ��ٰ汾������ú����ܹ��Ƶ���constantֵ����ֱ�ӷ���.
bool ConstantEvaluator::FastEvaluateAsRValue(const Expr *Exp, EvalInfo &Result)
{
	if (Result.evalstatus.kind == EvalStatus::ValueKind::IntKind)
	{
		if (const NumberExpr* Num = dynamic_cast<const NumberExpr*>(Exp))
		{
			Result.evalstatus.intVal = Num->getVal();
			Result.evalstatus.result = EvalStatus::Result::Constant;
			return true;
		}
		return false;
	}

	if (Result.evalstatus.kind == EvalStatus::ValueKind::BoolKind)
	{
		if (const BoolLiteral* BL = dynamic_cast<const BoolLiteral*>(Exp))
		{
			Result.evalstatus.boolVal = BL->getVal();
			Result.evalstatus.result = EvalStatus::Result::Constant;
			return true;
		}
		return false;
	}
	return false;
}

/// \brief ����evaluate���ռ�����
bool ConstantEvaluator::Evaluate(const Expr* Exp, EvalInfo &Result)
{
	if (Result.evalstatus.kind == ValueKind::IntKind)
	{
		IntExprEvaluator evaluator;
		evaluator.Evaluate(Exp, Result);
	}

	if (Result.evalstatus.kind == ValueKind::BoolKind)
	{
		BoolExpeEvaluator evaluator;
		evaluator.Evaluate(Exp, Result);
	}
	return true;
}