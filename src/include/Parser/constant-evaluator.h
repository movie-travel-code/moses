//===-----------------------constant-evaluator.h--------------------------===//
//
// This file implements the Expr constant evaluator.
//
// Constant expression evaluation produces four main results:
// * A success/failure flag indicating whether constant folding was successful.
//	 This is the 'bool' return value used by most of the code in this file.
//A 	 'false' return value indicates that constant folding has failed.
//
// * An evaluated result, valid only if constant folding has not faild.
//
//==----------------------------------------------------------------------===//
#ifndef CONSTANT_EVALUATOR_H
#define CONSTANT_EVALUATOR_H
#include "EvaluatedExprVisitor.h"
#include "ast.h"


namespace compiler {
namespace ast {
/// constant-evaluator
/// func add(lhs : int, rhs : int) -> int
/// {
///		return lhs + rhs * 2 - 40 + lhs * (rhs - rhs / 10);
/// }
/// const global = 10;
/// var num = add(global, 20) + 23;
class ConstantEvaluator {
  /// https://akrzemi1.wordpress.com/2011/05/06/compile-time-computations/
  /// http://clang.llvm.org/docs/InternalsManual.html#constant-folding-in-the-clang-ast
public:
  typedef EvalStatus::ValueKind ValueKind;
  typedef EvalStatus::Result Result;

  typedef EvalInfo::EvaluationMode EvaluationMode;

public:
  bool EvaluateAsRValue(ExprASTPtr Exp, EvalInfo &Result) const;

  bool EvaluateAsInt(ExprASTPtr Exp, int &Result) const;

  /// EvaluateAsBooleanCondition - Return true if this is a boolean constant
  /// which we can fold.
  bool EvaluateAsBooleanCondition(ExprASTPtr Exp, bool &Result) const;

  static bool FastEvaluateAsRValue(ExprASTPtr Exp, EvalInfo &Result);

  static bool Evaluate(ExprASTPtr Exp, EvalInfo &Result);

  bool HasSideEffects(const Expr *Exp) const;
};
} // namespace ast
} // namespace compiler
#endif