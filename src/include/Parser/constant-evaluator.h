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
//---------------------------------------------------------------------------//
//
// moses��constant folding��Ҫ�����ں�˵��Ż���������ǿ���Եģ�Ҳ����˵���
// expression���ܽ���constant folding�����ᱨ��moses��constant folding��Clang
// �е� potential constant expression�Ƚ����ơ�
//
// moses��constant folding��Ҫ��Ϊ�����֣�
// (1) BinaryExpression��cons tant-folding
//	���磺
//		������һ��
//		var num = 10;
//		...
//		// start can't
//folding����Ϊ��֪���м��Ƿ��num���и�ֵ��������Ҫ��;�ռ���num�ĸ�ֵ��Ϣ
//		// ���ڸ���
//		var start = num * 13 + 12;
//
//		����������
//		const num = 10;
//		...
//		//
//��ͬ��������һ���������num��const���ͣ�����ȷ���м�û�ж�����еĸ�ֵ�޸ģ����Կ���
//		// ��start constant-fold��142
//		var start = num * 13 + 12
//
//		����������
//		// ͬ��flag ����const�����ڼ䣬���Ὣ flag && isGreater()
//�۵���false���������
//		// isGreater()�����и����õĻ���Ҳ�������constant-folding.
//		var flag = false;
//		if (flag && isGreater()) {} else {}
//
//		�������ģ�
//		// ��isGreater()����û�и����õ�����£����Խ� flag&&isGreater()
//�۵���false. 		constant flag = false; 		if (flag && isGreater()) {} else {}
//
// (2) UnaryExpression��constant-folding
// ���磺
//		�������壩
//		const num = 10;
//		var start = ++num;	// start constant-folding��Ϊ11
//
//		����������
//		const num = 10;
//		var start = num--;	// start constant-folding��Ϊ10
//
//		�������ߣ�
//		var flag = false;
//		if !flag {} else {}
//
// (3) CallExpr��constant-folding
//	���磺
//		�������ˣ�
//		const parm1 = 10;
//		const parm2 = 11;
//		func add(lhs : int, rhs : int) -> int { return lhs + rhs; }
//		if add(parm1, parm2) < 10 {} else {}
//		���� add(parm1, parm2) < 10
//����constant-folding��false��Ҳ��������һ�� 		��else block��ִ�У�������С��code
//size�����ٵ�branch����С��ִ�д���
//
//	Note: ����CallExpr��constant-foldingʹ�����������ƣ�
//		��1�� ���������ǿ��Ƶ���
//		��2�� ֻ�е� return ���
//
//---------------------------------------------------------------------------//
#include "EvaluatedExprVisitor.h"
#include "ast.h"


namespace compiler {
namespace ast {
/// ��ʱʵ�������µ�constant-evaluator
/// func add(lhs : int, rhs : int) -> int
/// {
///		return lhs + rhs * 2 - 40 + lhs * (rhs - rhs / 10);
/// }
/// const global = 10;
/// var num = add(global, 20) + 23;
/// moses���Խ�num evaluate�õ�213.
class ConstantEvaluator {
  /// https://akrzemi1.wordpress.com/2011/05/06/compile-time-computations/
  /// http://clang.llvm.org/docs/InternalsManual.html#constant-folding-in-the-clang-ast
  /// ������ʱдһ��������evaluate����
public:
  typedef EvalStatus::ValueKind ValueKind;
  typedef EvalStatus::Result Result;

  typedef EvalInfo::EvaluationMode EvaluationMode;

public:
  /// To Do:ʵ��Expr��compile-time��evaluate
  ///----------------------------nonsense for coding---------------------
  /// ����Ӧ�÷���Clangʵ��EvaluateAsRValue()�������ú����ǳ�ǿ��
  ///--------------------------------------------------------------------
  // To Do: û��ʵ��
  bool EvaluateAsRValue(ExprASTPtr Exp, EvalInfo &Result) const;

  bool EvaluateAsInt(ExprASTPtr Exp, int &Result) const;

  /// EvaluateAsBooleanCondition - Return true if this is a boolean constant
  /// which we can fold.
  bool EvaluateAsBooleanCondition(ExprASTPtr Exp, bool &Result) const;

  static bool FastEvaluateAsRValue(ExprASTPtr Exp, EvalInfo &Result);

  static bool Evaluate(ExprASTPtr Exp, EvalInfo &Result);

  /// \brief �жϵ�ǰ���ʽ�Ƿ���sideeffects.
  bool HasSideEffects(const Expr *Exp) const;
};
} // namespace ast
} // namespace compiler
#endif