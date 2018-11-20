//===------------------------EvaluatedExprVisitor.h-----------------------===//
//
// This file defines the EvaluateExprVisitor class.
// 
//===---------------------------------------------------------------------===//

#ifndef EVALUATED_EXPR_VISITOR_H
#define EVALUATED_EXPR_VISITOR_H
#include "ast.h"
#include <iostream>
#include <typeinfo>
#include <utility>

namespace compiler {
namespace ast {
struct EvalStatus {
  enum ValueKind { IntKind, BoolKind, AnonymousKind };

  enum Result {
    Constant,
    NotConstant,
    Pending
  };
  
  bool HasSideEffects;

  EvalStatus() : HasSideEffects(false), result(Result::Pending) {}

  // hasSideEffects - Return true if the evaluated expression has side
  // effects.
  bool hasSideEffects() const { return HasSideEffects; }

  unsigned intVal;
  bool boolVal;
  // AnonymousTreeVal;

  ValueKind kind;
  Result result;
};

struct EvalInfo {
  // (1) var num = start + end + mid;
  // (2) var num = func(num);
  unsigned CallStackDepth;

  EvalStatus evalstatus;

  enum EvaluationMode {
    EM_PotentialConstantExpression,

    /// Fold the expression to a constant. Stop if we hit a side-effect
    /// that we can't model.	-Clang
    EM_ConstantFold
  } EvalMode;
  /// Are we checking whether the expression is a potential constant
  /// expression?
  bool checkingPotentialConstantExpression() const {
    return EvalMode == EM_PotentialConstantExpression;
  }

  EvalInfo(EvalStatus::ValueKind vk, EvaluationMode Mode)
      : CallStackDepth(0), EvalMode(Mode) {
    evalstatus.kind = vk;
  }

  // Note that we had a side-effect.
  bool noteSideEffect();
};

class EvaluatedExprVisitorBase {
public:
  std::vector<std::pair<std::string, EvalInfo>> ActiveStack;

  std::vector<std::pair<int, unsigned>> ActiveBookingInfo;

public:
  using ValueKind = EvalStatus::ValueKind;
  using Result = EvalStatus::Result;

  using EvaluationMode = EvalInfo::EvaluationMode;

public:
  virtual bool Evaluate(ExprASTPtr expr, EvalInfo &info);

  virtual bool EvalBinaryExpr(BinaryPtr B, EvalInfo &info, EvalInfo &lhs,
                              EvalInfo &rhs) = 0;

  virtual void handleEvalCallTail();

  virtual ReturnStmtPtr handleEvalCallStart(CallExprPtr CE);

  virtual bool EvalUnaryExpr(UnaryPtr U, EvalInfo &info, EvalInfo &subVal) = 0;

  // virtual bool EvalAnonymousInitExpr(const AnonymousInitExpr* Exp, EvalInfo
  // &info) = 0;
  virtual bool EvalMemberExpr(MemberExprPtr ME, EvalInfo &info) = 0;

  virtual bool EvalCallExpr(CallExprPtr CE, EvalInfo &info);

  virtual bool EvalDeclRefExpr(DeclRefExprPtr DRE, EvalInfo &info);
};

class IntExprEvaluator final : public EvaluatedExprVisitorBase {
public:
  bool EvalBinaryExpr(BinaryPtr B, EvalInfo &info, EvalInfo &lhs,
                      EvalInfo &rhs) override;

  bool EvalUnaryExpr(UnaryPtr U, EvalInfo &info, EvalInfo &subVal) override;

  bool EvalMemberExpr(MemberExprPtr ME, EvalInfo &info) override;
};

class BoolExprEvaluator final : public EvaluatedExprVisitorBase {
private:
public:
  /// \brief
  bool EvalBinaryExpr(BinaryPtr B, EvalInfo &info, EvalInfo &lhs,
                      EvalInfo &rhs) override;

  bool EvalUnaryExpr(UnaryPtr U, EvalInfo &info, EvalInfo &subVal) override;

  bool EvalMemberExpr(MemberExprPtr ME, EvalInfo &info) override;
};
} // namespace ast
} // namespace compiler

#endif