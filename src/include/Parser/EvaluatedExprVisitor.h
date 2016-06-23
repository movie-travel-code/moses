//===------------------------EvaluatedExprVisitor.h-----------------------===//
//
// This file defines the EvaluateExprVisitor class.
// 
// 不同于Clang，Clang为了提升代码效率使用CRTP模式实现多态，这里我们使用虚函数调用
// 来实现该机制。其中该visitor使用visitor pattern，遍历给定的子树根节点，并执行
// 相应的行为。
// 例如：
//                  +
//				  /   \
//				 -	   *
//			    / \   /  \
//			  num 1 start 3
//
// 对于IntExprEvaluator来说，VisitBinaryOperator()来说，就是 lhs + rhs. 中间会
// 递归遍历 lhs 和 rhs 然后，得出每个部分的值，然后相加。
// 对于StringExprEvaluator来说，VisitBinaryOperator()来说，就是 "hello" + "world".
// 
//				add()
//			   /     \
//			parm    body
//          /           \
//   {模拟计算实参值} {模拟运行函数体}
// 
// 其中对于 IntExprEvaluator 和 StringExprEvaluator 来说都会进行CallExpr进行
// evaluate，在moses中限制 eval 深度最深为 1
//===---------------------------------------------------------------------===//

#ifndef EVALUATED_EXPR_VISITOR_H
#define EVALUATED_EXPR_VISITOR_H
#include <iostream>
#include <utility>
#include <typeinfo>
#include "ast.h"

namespace compiler
{
	namespace ast
	{	
		struct EvalStatus
		{
			enum ValueKind
			{
				IntKind,
				BoolKind,
				AnonymousKind
			};

			enum Result
			{
				Constant,
				NotConstant,
				// 由于当前表达式依赖于还没有进行evaluate的符号，还不能被evaluate，
				Pending
			};
			/// \brief 待进行evaluated的表达式是否有副作用。
			/// 例如： (f() && 0) 可以被折叠成0，但是函数f()有副作用
			bool HasSideEffects;

			EvalStatus() : HasSideEffects(false), result(Result::Pending){}

			// hasSideEffects - Return true if the evaluated expression has side 
			// effects.
			bool hasSideEffects() const
			{
				return HasSideEffects;
			}

			// Note: clang中间提供了一个ExprResult{}结构体，这里简单起见，不会提供。
			// 如今moses只提供bool与int类型，所以不需要特别复杂的值表示，后面在丰富
			// moses的时候就需要提供一个完善的值表示。
			int intVal;
			bool boolVal;
			// To Do: AnonymousType是值暂时不支持，AnonymousType的值需要用树来存储。
			// AnonymousTreeVal;

			ValueKind kind;
			Result result;
		};

		/// EvalInfo - EvalInfo不同于EvalStatus，EvalInfo表示的是推导过程中，记录Eval环境信息
		/// EvalStatus表示的evaluate的结果。
		/// Note: 对于CallExpr来说，constant-evaluator只会推导一次调用（这里强制，函数只能有
		/// 一条return语句）
		struct EvalInfo
		{
			// 推导的时所在栈帧的深度，
			// (1) var num = start + end + mid; 这里面Depth是0
			// (2) var num = func(num); 这里面Depth是1
			unsigned CallStackDepth;

			EvalStatus evalstatus;

			enum EvaluationMode
			{
				/// 该模式下，只有遇到不可evaluate的情况才会停下，即使有side-effect.
				EM_PotentialConstantExpression,

				/// Fold the expression to a constant. Stop if we hit a side-effect
				/// that we can't model.	-Clang
				/// 该模式是moses constant-evaluator主要的模式。
				EM_ConstantFold
			} EvalMode;
			/// Are we checking whether the expression is a potential constant 
			/// expression?
			bool checkingPotentialConstantExpression() const
			{
				return EvalMode == EM_PotentialConstantExpression;
			}

			// To Do: 对于Compile-time的推导，应该有步数限制，这里没有设置steps limit
			// 只是对eval的函数调用层数有所限制。 
			EvalInfo(EvalStatus::ValueKind vk, EvaluationMode Mode)
				: CallStackDepth(0), EvalMode(Mode)
			{
				evalstatus.kind = vk;
			}

			// Note that we had a side-effect.
			bool noteSideEffect();
		};

		class EvaluatedExprVisitorBase : public StatementAST::Visitor
		{
		public:
			// 在对 CallExpr 进行eval的时候，需要对实参进行推导，所以在对函数体进行eval的
			// 时候，需要记录实参的值（就像真正的函数调用一样）
			// 例如：
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
			//	level0:		对实参进行eval，得到value_list如下，
			//			value_list <start, 10> <end, 11> 
			//
			//	level1:		return parm1 + parm2 * length;
			//
			//	在对return语句进行eval的时候，需要对expression表达式中的变量进行eval，所以就需要记录到以前
			//	eval得到的结果。
			// Note: 现在我们对CallExpr只能eval一层，以后也有可能eval多层
			//		0：				add(start, end)				记录第0层的实参值
			//							   ||
			//							   ||
			//							   \/
			//		1：		return parm1 + parm2 * length;		有可能需要记录第1层的实参值
			//						//				\\
			//					   //				 \\
			//
			//		2：	有可能有第三层，所以需要					有可能需要记录第2层的实参值

			// To Do: 使用栈来记录实参值（模拟真实程序运行）
			// 例如： 动态扩展的active stack，每一个栈帧都会存储实参值对
			//	$$<start, 11> <end, 10> $$ <flag, false> $$ <>
			//  <level0, 2> <level2, 1> 
			// Note: moses对CallExpr的推导暂时只有一层。

			// （1） 该vector存储active stack info.
			std::vector<std::pair<std::string, EvalInfo>> ActiveStack;

			// （2） 该vector存储stack bookkeeping info，level以及每层有多少个实参值.
			std::vector<std::pair<int, unsigned>> ActiveBookingInfo;

		public:
			typedef EvalStatus::ValueKind ValueKind;
			typedef EvalStatus::Result Result;

			typedef EvalInfo::EvaluationMode EvaluationMode;
		public:
			virtual bool Evaluate(ExprASTPtr expr, EvalInfo& info);

			virtual bool EvalBinaryExpr(BinaryPtr B, EvalInfo &info, EvalInfo &lhs, 
				EvalInfo &rhs) = 0;

			/// \brief 该函数用于在EvalCall()的结尾处理bookkeeping info
			virtual void handleEvalCallTail();

			/// \brief 该函数用于在EvalCall之前进行检查
			virtual ReturnStmtPtr handleEvalCallStart(CallExprPtr CE);

			virtual bool EvalUnaryExpr(UnaryPtr U, EvalInfo &info, EvalInfo &subVal) = 0;

			// virtual bool EvalAnonymousInitExpr(const AnonymousInitExpr* Exp, EvalInfo &info) = 0;
			virtual bool EvalMemberExpr(MemberExprPtr ME, EvalInfo &info) = 0;

			/// \brief 对于CallExpr，要求被调用函数只能有一个return语句.
			virtual bool EvalCallExpr(CallExprPtr CE, EvalInfo &info);

			/// \brief 暂时moses的constant-evaluator机制还不是很强大，只支持const变量的eval。
			/// 不会沿途收集对num的赋值信息。
			virtual bool EvalDeclRefExpr(DeclRefExprPtr DRE, EvalInfo &info);
		};

		/// \biref 遍历Expression用于Int值的evaluate.
		class IntExprEvaluator final : public EvaluatedExprVisitorBase
		{
		public:
			/// \brief 递归遍历 LHS 和 RHS，执行运算符对应的操作。
			/// 基本的语义分析已经完成（例如TypeChecking等），不会出现类型错误
			bool EvalBinaryExpr(BinaryPtr B, EvalInfo &info, EvalInfo &lhs, EvalInfo &rhs) override;

			/// \brief 用于Evaluate Unary Expression.
			/// 例如： -num.
			bool EvalUnaryExpr(UnaryPtr U, EvalInfo &info, EvalInfo &subVal) override;

			/// \brief 如果MemberExpr中的成员声明时const类型，则可以直接使用相应的值作为constant值.
			bool EvalMemberExpr(MemberExprPtr ME, EvalInfo &info) override;
		};

		/// \brief 遍历Expression用于Bool值的evaluate.
		class BoolExprEvaluator final : public EvaluatedExprVisitorBase
		{
		private:
		public:
			/// \brief 对于boolean类型的推导，对应的BinaryExpr只能是逻辑运算符。
			bool EvalBinaryExpr(BinaryPtr B, EvalInfo &info, EvalInfo &lhs, EvalInfo &rhs) override;

			/// \brief 对于boolean类型来说，单目运算符只有!
			bool EvalUnaryExpr(UnaryPtr U, EvalInfo &info, EvalInfo &subVal) override;

			/// \brief 如果MemberExpr中的成员声明时const类型，则可以直接使用相应的值作为constant值.
			bool EvalMemberExpr(MemberExprPtr ME, EvalInfo &info) override;
		};
	}
}

#endif