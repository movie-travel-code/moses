//===-----------------------constant-evaluator.h--------------------------===//
// 
// This file implements the Expr constant evaluator. 
//
// Constant expression evaluation produces four main results:
// * A success/failure flag indicating whether constant folding was successful.
//	 This is the 'bool' return value used by most of the code in this file. A
//	 'false' return value indicates that constant folding has failed.
//	
// * An evaluated result, valid only if constant folding has not faild.
//
//==----------------------------------------------------------------------===//
#ifndef CONSTANT_EVALUATOR_H
#define CONSTANT_EVALUATOR_H
//---------------------------------------------------------------------------//
// 
// moses的constant folding主要是用于后端的优化，并不是强制性的，也就是说如果
// expression不能进行constant folding并不会报错。moses的constant folding与Clang
// 中的 potential constant expression比较相似。
// 
// moses的constant folding主要分为两部分：
// (1) BinaryExpression的cons tant-folding
//	例如：
//		用例（一）
//		var num = 10;
//		...
//		// start can't folding，因为不知道中间是否对num进行赋值操作，需要沿途收集对num的赋值信息
//		// 过于复杂
//		var start = num * 13 + 12; 
//		
//		用例（二）
//		const num = 10;
//		...
//		// 不同于用例（一），这里的num是const类型，可以确保中间没有对其进行的赋值修改，所以可以
//		// 将start constant-fold成142
//		var start = num * 13 + 12
//		
//		用例（三）
//		// 同理flag 不是const，简单期间，不会将 flag && isGreater() 折叠成false，另外如果
//		// isGreater()函数有副作用的话，也不会进行constant-folding.
//		var flag = false;
//		if (flag && isGreater()) {} else {}
//
//		用例（四）
//		// 在isGreater()函数没有副作用的情况下，可以将 flag&&isGreater() 折叠成false.
//		constant flag = false;
//		if (flag && isGreater()) {} else {}
//		
// (2) UnaryExpression的constant-folding
// 例如：
//		用例（五）
//		const num = 10;
//		var start = ++num;	// start constant-folding成为11
//
//		用例（六）
//		const num = 10;
//		var start = num--;	// start constant-folding成为10
//
//		用例（七）
//		var flag = false;
//		if !flag {} else {}
//
// (3) CallExpr的constant-folding
//	例如：
//		用例（八）
//		const parm1 = 10;
//		const parm2 = 11;
//		func add(lhs : int, rhs : int) -> int { return lhs + rhs; }
//		if add(parm1, parm2) < 10 {} else {}
//		其中 add(parm1, parm2) < 10 可以constant-folding到false，也就是这是一个
//		单else block的执行，带来更小的code size，更少的branch，更小的执行次数
//
//	Note: 对于CallExpr的constant-folding使用有如下限制：
//		（1） 函数必须是可推导的
//		（2） 只有单 return 语句
//		
//---------------------------------------------------------------------------//
#include "ast.h"
#include "EvaluatedExprVisitor.h"

namespace compiler
{
	namespace ast
	{
		class ConstantEvaluator
		{
			/// https://akrzemi1.wordpress.com/2011/05/06/compile-time-computations/ 
			/// http://clang.llvm.org/docs/InternalsManual.html#constant-folding-in-the-clang-ast
			/// 这里暂时写一个辣鸡的evaluate机制
		public:
			typedef EvalStatus::ValueKind ValueKind;
			typedef EvalStatus::Result Result;

			typedef EvalInfo::EvaluationMode EvaluationMode;
		public:
			/// To Do:实现Expr在compile-time的evaluate
			///----------------------------nonsense for coding---------------------
			/// 这里应该仿照Clang实现EvaluateAsRValue()函数，该函数非常强大
			///--------------------------------------------------------------------
			// To Do: 没有实现
			bool EvaluateAsRValue(const Expr *Exp, EvalInfo &Result) const;

			bool EvaluateAsInt(const Expr *Exp, int &Result) const;

			/// EvaluateAsBooleanCondition - Return true if this is a boolean constant 
			/// which we can fold. 
			bool EvaluateAsBooleanCondition(const Expr *Exp, bool &Result) const;

			static bool FastEvaluateAsRValue(const Expr *Exp, EvalInfo &Result);

			static bool Evaluate(const Expr* Exp, EvalInfo &Result);

			/// \brief 判断当前表达式是否有sideeffects.
			bool HasSideEffects(const Expr* Exp) const;
		};
	}
}
#endif