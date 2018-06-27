//===------------------------EvaluatedExprVisitor.h-----------------------===//
//
// This file defines the EvaluateExprVisitor class.
//
// ��ͬ��Clang��ClangΪ����������Ч��ʹ��CRTPģʽʵ�ֶ�̬����������ʹ���麯������
// ��ʵ�ָû��ơ����и�visitorʹ��visitor pattern�������������������ڵ㣬��ִ��
// ��Ӧ����Ϊ��
// ���磺
//                  +
//				  /   \
//				 -	   *
//			    / \   /  \
//			  num 1 start 3
//
// ����IntExprEvaluator��˵��VisitBinaryOperator()��˵������ lhs + rhs. �м��
// �ݹ���� lhs �� rhs
// Ȼ�󣬵ó�ÿ�����ֵ�ֵ��Ȼ����ӡ�
// ����StringExprEvaluator��˵��VisitBinaryOperator()��˵������ "hello" + "world".
//
//				add()
//			   /     \
//			parm    body
//          /           \
//   {ģ�����ʵ��ֵ} {ģ�����к�����}
//
// ���ж��� IntExprEvaluator �� StringExprEvaluator ��˵�������CallExpr����
// evaluate����moses������ eval �������Ϊ 1
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
    // ���ڵ�ǰ���ʽ�����ڻ�û�н���evaluate�ķ��ţ������ܱ�evaluate��
    Pending
  };
  /// \brief ������evaluated�ı��ʽ�Ƿ��и����á�
  /// ���磺 (f() && 0) ���Ա��۵���0�����Ǻ���f()�и�����
  bool HasSideEffects;

  EvalStatus() : HasSideEffects(false), result(Result::Pending) {}

  // hasSideEffects - Return true if the evaluated expression has side
  // effects.
  bool hasSideEffects() const { return HasSideEffects; }

  // Note: clang�м��ṩ��һ��ExprResult{}�ṹ�壬���������������ṩ��
  // ���mosesֻ�ṩbool��int���ͣ����Բ���Ҫ�ر��ӵ�ֵ��ʾ�������ڷḻ
  // moses��ʱ�����Ҫ�ṩһ�����Ƶ�ֵ��ʾ��
  unsigned intVal;
  bool boolVal;
  // To Do: AnonymousType��ֵ��ʱ��֧�֣�AnonymousType��ֵ��Ҫ�������洢��
  // AnonymousTreeVal;

  ValueKind kind;
  Result result;
};

/// EvalInfo - EvalInfo��ͬ��EvalStatus��EvalInfo��ʾ�����Ƶ������У���¼Eval������Ϣ
/// EvalStatus��ʾ��evaluate�Ľ����
/// Note: ����CallExpr��˵��constant-evaluatorֻ���Ƶ�һ�ε��ã�����ǿ�ƣ�����ֻ����
/// һ��return��䣩
struct EvalInfo {
  // �Ƶ���ʱ����ջ֡����ȣ�
  // (1) var num = start + end + mid; ������Depth��0
  // (2) var num = func(num); ������Depth��1
  unsigned CallStackDepth;

  EvalStatus evalstatus;

  enum EvaluationMode {
    /// ��ģʽ�£�ֻ����������evaluate������Ż�ͣ�£���ʹ��side-effect.
    EM_PotentialConstantExpression,

    /// Fold the expression to a constant. Stop if we hit a side-effect
    /// that we can't model.	-Clang
    /// ��ģʽ��moses constant-evaluator��Ҫ��ģʽ��
    EM_ConstantFold
  } EvalMode;
  /// Are we checking whether the expression is a potential constant
  /// expression?
  bool checkingPotentialConstantExpression() const {
    return EvalMode == EM_PotentialConstantExpression;
  }

  // To Do: ����Compile-time���Ƶ���Ӧ���в������ƣ�����û������steps limit
  // ֻ�Ƕ�eval�ĺ������ò����������ơ�
  EvalInfo(EvalStatus::ValueKind vk, EvaluationMode Mode)
      : CallStackDepth(0), EvalMode(Mode) {
    evalstatus.kind = vk;
  }

  // Note that we had a side-effect.
  bool noteSideEffect();
};

class EvaluatedExprVisitorBase {
public:
  // �ڶ� CallExpr ����eval��ʱ����Ҫ��ʵ�ν����Ƶ��������ڶԺ��������eval��
  // ʱ����Ҫ��¼ʵ�ε�ֵ�����������ĺ�������һ����
  // ���磺
  // const length = 20;
  //	func add(parm1 : int, parm2 : int) -> int
  //	{
  //		return parm1 + parm2 * length;
  //	}
  //
  //	const start = 10;
  //	const end = 11;
  //	var num = add(start, end);
  //
  //	level0:		��ʵ�ν���eval���õ�value_list���£�
  //			value_list <start, 10> <end, 11>
  //
  //	level1:		return parm1 + parm2 * length;
  //
  //	�ڶ�return������eval��ʱ����Ҫ��expression���ʽ�еı�������eval�����Ծ���Ҫ��¼����ǰ
  //	eval�õ��Ľ����
  // Note: �������Ƕ�CallExprֻ��evalһ�㣬�Ժ�Ҳ�п���eval���
  //		0��				add(start, end)
  //��¼��0���ʵ��ֵ
  //							   ||
  //							   ||
  //							   \/
  //		1��		return parm1 + parm2 * length;
  //�п�����Ҫ��¼��1���ʵ��ֵ
  //						// \\
			//					   //				 \\
			//
  //		2��	�п����е����㣬������Ҫ
  //�п�����Ҫ��¼��2���ʵ��ֵ

  // To Do: ʹ��ջ����¼ʵ��ֵ��ģ����ʵ�������У�
  // ���磺 ��̬��չ��active stack��ÿһ��ջ֡����洢ʵ��ֵ��
  //	$$<start, 11> <end, 10> $$ <flag, false> $$ <>
  //  <level0, 2> <level2, 1>
  // Note: moses��CallExpr���Ƶ���ʱֻ��һ�㡣

  // ��1�� ��vector�洢active stack info.
  std::vector<std::pair<std::string, EvalInfo>> ActiveStack;

  // ��2�� ��vector�洢stack bookkeeping info��level�Լ�ÿ���ж��ٸ�ʵ��ֵ.
  std::vector<std::pair<int, unsigned>> ActiveBookingInfo;

public:
  typedef EvalStatus::ValueKind ValueKind;
  typedef EvalStatus::Result Result;

  typedef EvalInfo::EvaluationMode EvaluationMode;

public:
  virtual bool Evaluate(ExprASTPtr expr, EvalInfo &info);

  virtual bool EvalBinaryExpr(BinaryPtr B, EvalInfo &info, EvalInfo &lhs,
                              EvalInfo &rhs) = 0;

  /// \brief �ú���������EvalCall()�Ľ�β����bookkeeping info
  virtual void handleEvalCallTail();

  /// \brief �ú���������EvalCall֮ǰ���м��
  virtual ReturnStmtPtr handleEvalCallStart(CallExprPtr CE);

  virtual bool EvalUnaryExpr(UnaryPtr U, EvalInfo &info, EvalInfo &subVal) = 0;

  // virtual bool EvalAnonymousInitExpr(const AnonymousInitExpr* Exp, EvalInfo
  // &info) = 0;
  virtual bool EvalMemberExpr(MemberExprPtr ME, EvalInfo &info) = 0;

  /// \brief ����CallExpr��Ҫ�󱻵��ú���ֻ����һ��return���.
  virtual bool EvalCallExpr(CallExprPtr CE, EvalInfo &info);

  /// \brief ��ʱmoses��constant-evaluator���ƻ����Ǻ�ǿ��ֻ֧��const������eval��
  /// ������;�ռ���num�ĸ�ֵ��Ϣ��
  virtual bool EvalDeclRefExpr(DeclRefExprPtr DRE, EvalInfo &info);
};

/// \biref ����Expression����Intֵ��evaluate.
class IntExprEvaluator final : public EvaluatedExprVisitorBase {
public:
  /// \brief �ݹ���� LHS �� RHS��ִ���������Ӧ�Ĳ�����
  /// ��������������Ѿ���ɣ�����TypeChecking�ȣ�������������ʹ���
  bool EvalBinaryExpr(BinaryPtr B, EvalInfo &info, EvalInfo &lhs,
                      EvalInfo &rhs) override;

  /// \brief ����Evaluate Unary Expression.
  /// ���磺 -num.
  bool EvalUnaryExpr(UnaryPtr U, EvalInfo &info, EvalInfo &subVal) override;

  /// \brief ���MemberExpr�еĳ�Ա����ʱconst���ͣ������ֱ��ʹ����Ӧ��ֵ��Ϊconstantֵ.
  bool EvalMemberExpr(MemberExprPtr ME, EvalInfo &info) override;
};

/// \brief ����Expression����Boolֵ��evaluate.
class BoolExprEvaluator final : public EvaluatedExprVisitorBase {
private:
public:
  /// \brief
  /// ����boolean���͵��Ƶ�����Ӧ��BinaryExprֻ�����߼��������
  bool EvalBinaryExpr(BinaryPtr B, EvalInfo &info, EvalInfo &lhs,
                      EvalInfo &rhs) override;

  /// \brief ����boolean������˵����Ŀ�����ֻ��!
  bool EvalUnaryExpr(UnaryPtr U, EvalInfo &info, EvalInfo &subVal) override;

  /// \brief ���MemberExpr�еĳ�Ա����ʱconst���ͣ������ֱ��ʹ����Ӧ��ֵ��Ϊconstantֵ.
  bool EvalMemberExpr(MemberExprPtr ME, EvalInfo &info) override;
};
} // namespace ast
} // namespace compiler

#endif