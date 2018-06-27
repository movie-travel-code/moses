//===------------------------constant-evaluator.cpp-----------------------===//
//
// This file implements the Expr constant evaluator.
//
//===---------------------------------------------------------------------===//
#include "../../include/Parser/constant-evaluator.h"
using namespace compiler::ast;
/// \brief �ú����������Ժ��������Ժ��������Ƶ���
/// �ú���������Ƶ��Ŀ��ٰ汾(FastEvaluateAsRValue)��ǿ���汾(EvaluateAsRValue).
bool ConstantEvaluator::EvaluateAsRValue(ExprASTPtr Exp,
                                         EvalInfo &Result) const {
  // ���ÿ���evaluate�汾������ܹ�����constant-evalute����ֱ�ӷ���
  if (FastEvaluateAsRValue(Exp, Result))
    return true;

  // ����ǿ���汾��Ĭ��evaluate�Ĳ�����32
  if (Evaluate(Exp, Result))
    return true;
  return false;
}

/// \brief ���Exp�ܷ��Ƶ���Ϊһ��intֵ�����������ŵ�Result��.
/// ����EvaluateAsRValue(const Expr *Exp, EvalStatus &Result)����.
bool ConstantEvaluator::EvaluateAsInt(ExprASTPtr Exp, int &Result) const {
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
bool ConstantEvaluator::EvaluateAsBooleanCondition(ExprASTPtr Exp,
                                                   bool &Result) const {
  // �ñ��ʽ����bool����
  if (Exp->getType()->getKind() != TypeKind::BOOL)
    return false;
  EvalInfo info(EvalStatus::ValueKind::BoolKind,
                EvaluationMode::EM_ConstantFold);
  // constant evaluator
  EvaluateAsRValue(Exp, info);

  // �����Ƶ�
  if (info.evalstatus.result == EvalStatus::Result::NotConstant)
    return false;
  Result = info.evalstatus.boolVal;
  return true;
}

/// \brief FastEvaluateAsRValue�Ŀ��ٰ汾������ú����ܹ��Ƶ���constantֵ����ֱ�ӷ���.
bool ConstantEvaluator::FastEvaluateAsRValue(ExprASTPtr Exp, EvalInfo &Result) {
  if (Result.evalstatus.kind == EvalStatus::ValueKind::IntKind) {
    if (std::shared_ptr<NumberExpr> Num =
            std::dynamic_pointer_cast<NumberExpr>(Exp)) {
      Result.evalstatus.intVal = Num->getVal();
      Result.evalstatus.result = EvalStatus::Result::Constant;
      return true;
    }
  }

  if (Result.evalstatus.kind == EvalStatus::ValueKind::BoolKind) {
    if (std::shared_ptr<BoolLiteral> BL =
            std::dynamic_pointer_cast<BoolLiteral>(Exp)) {
      Result.evalstatus.boolVal = BL->getVal();
      Result.evalstatus.result = EvalStatus::Result::Constant;
      return true;
    }
  }
  return false;
}

/// \brief ����evaluate���ռ�����
bool ConstantEvaluator::Evaluate(ExprASTPtr Exp, EvalInfo &Result) {
  if (Result.evalstatus.kind == ValueKind::IntKind) {
    IntExprEvaluator evaluator;
    if (!evaluator.Evaluate(Exp, Result)) {
      Result.evalstatus.result = Result::NotConstant;
      return false;
    }
  }

  if (Result.evalstatus.kind == ValueKind::BoolKind) {
    BoolExprEvaluator evaluator;
    if (!evaluator.Evaluate(Exp, Result)) {
      Result.evalstatus.result = Result::NotConstant;
      return false;
    }
  }
  return true;
}