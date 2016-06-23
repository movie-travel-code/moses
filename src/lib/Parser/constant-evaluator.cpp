//===------------------------constant-evaluator.cpp-----------------------===//
//
// This file implements the Expr constant evaluator.
//
//===---------------------------------------------------------------------===//
#include "../../include/Parser/constant-evaluator.h"
using namespace compiler::ast;
/// \brief 该函数是总括性函数用来对函数进行推导。
/// 该函数会调用推导的快速版本(FastEvaluateAsRValue)和强化版本(EvaluateAsRValue).
bool ConstantEvaluator::EvaluateAsRValue(ExprASTPtr Exp, EvalInfo &Result) const
{
	// 调用快速evaluate版本，如果能够进行constant-evalute，则直接返回
	if (FastEvaluateAsRValue(Exp, Result))
		return true;

	// 调用强化版本，默认evaluate的步数是32
	if (Evaluate(Exp, Result))
		return true;
	return false;
}

/// \brief 检查Exp能否推导成为一个int值，并将结果存放到Result中.
/// 调用EvaluateAsRValue(const Expr *Exp, EvalStatus &Result)函数.
bool ConstantEvaluator::EvaluateAsInt(ExprASTPtr Exp, int &Result) const
{
	// 该表达式不是整型
	if (Exp->getType()->getKind() != TypeKind::INT)
		return false;

	EvalInfo info(ValueKind::IntKind, EvaluationMode::EM_ConstantFold);
	// constant evaluator
	EvaluateAsRValue(Exp, info);

	// 不可推导
	if (info.evalstatus.result == Result::NotConstant)
		return false;
	Result = info.evalstatus.intVal;
	return true;
}

/// \brief 检查Exp能否推导成为一个bool值，并将结果存放到Result中.
/// 调用EvaluateAsRValue(const Expr *Exp, EvalStatus &Result)函数.
bool ConstantEvaluator::EvaluateAsBooleanCondition(ExprASTPtr Exp, bool &Result) const
{
	// 该表达式不是bool类型
	if (Exp->getType()->getKind() != TypeKind::BOOL)
		return false;
	EvalInfo info(EvalStatus::ValueKind::BoolKind, EvaluationMode::EM_ConstantFold);
	// constant evaluator
	EvaluateAsRValue(Exp, info);

	// 不可推导
	if (info.evalstatus.result == EvalStatus::Result::NotConstant)
		return false;
	Result = info.evalstatus.boolVal;
	return true;
}

/// \brief FastEvaluateAsRValue的快速版本，如果该函数能够推导出constant值，则直接返回.
bool ConstantEvaluator::FastEvaluateAsRValue(ExprASTPtr Exp, EvalInfo &Result)
{
	if (Result.evalstatus.kind == EvalStatus::ValueKind::IntKind)
	{
		if (std::shared_ptr<NumberExpr> Num = std::dynamic_pointer_cast<NumberExpr>(Exp))
		{
			Result.evalstatus.intVal = Num->getVal();
			Result.evalstatus.result = EvalStatus::Result::Constant;
			return true;
		}
	}

	if (Result.evalstatus.kind == EvalStatus::ValueKind::BoolKind)
	{
		if (std::shared_ptr<BoolLiteral> BL = std::dynamic_pointer_cast<BoolLiteral>(Exp))
		{
			Result.evalstatus.boolVal = BL->getVal();
			Result.evalstatus.result = EvalStatus::Result::Constant;
			return true;
		}
	}
	return false;
}

/// \brief 进行evaluate的终极函数
bool ConstantEvaluator::Evaluate(ExprASTPtr Exp, EvalInfo &Result)
{
	if (Result.evalstatus.kind == ValueKind::IntKind)
	{
		IntExprEvaluator evaluator;
		if (!evaluator.Evaluate(Exp, Result))
		{
			Result.evalstatus.result = Result::NotConstant;
			return false;
		}			
	}

	if (Result.evalstatus.kind == ValueKind::BoolKind)
	{
		BoolExprEvaluator evaluator;
		if (!evaluator.Evaluate(Exp, Result))
		{
			Result.evalstatus.result = Result::NotConstant;
			return false;
		}
	}
	return true;
}