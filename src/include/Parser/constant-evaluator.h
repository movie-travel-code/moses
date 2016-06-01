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
// (1) BinaryExpression的constant-folding
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
class ConstantEvaluator
{
	/// EvalStatus is a struct with detailed info about an evaluation in progress. -Clang
	/// 最新版的Clang提供了一个EvaluateExpressionVisitor模块，提供更加强大的evaluate.
	/// 这个结构体是为compile-time的evaluate服务的，moses尝试提供一个像clang一样强大的
	/// compile-time的evaluate机制。关于expression evaluated这里有一个非常不错的文章，
	/// https://akrzemi1.wordpress.com/2011/05/06/compile-time-computations/ 这篇文章介绍了
	/// compile-time computation.
	/// 以及http://clang.llvm.org/docs/InternalsManual.html#constant-folding-in-the-clang-ast
	/// 这里暂时写一个辣鸡的evaluate机制
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

		ValueKind kind;
		Result result;
	};

	/// EvalInfo - This ia a private struct used by the evaluatotr to capture
	/// information about a subexpression as it is folded. It retains
	/// information about the AST context, but also maintains information about
	/// the folded expression.
	///
	/// If an expression could be evaluated, it is still possible it is not a C
	/// "integer constant expression" or constant expression. If not, this struct 
	/// captures information about how and why not.
	///
	/// One bit of information passed *into* the request for constant folding
	/// indicates whether the subexpression is "evaluated" or not according to C
	/// C rules. For example, the RHS of (0 && foo()) is not evaluated. We can 
	/// evaluate the expression regardless of what the RHS is, but C only allows 
	/// certain things in certain situations.
	// To Do: 没有实现
	struct EvalInfo
	{
		// EvalStatus - Contains information about the evaluation.
		//Expr::EvalStatus &EvalStatus;

		// CurrentCall - The top of the constexpr call stack.
		//CallExpr *CE;

		// CallStackDepth - The number of calls in the call stack right now.
		unsigned CallStackDepth;

		// StepsLeft - The remaining number of evaluation steps we're permitted
		// to perform. This is essentially a limit for the number of statements
		// we will evaluate.
		//--------------------------nonsense for coding---------------------
		// 这个和Clang静态分析很相似，就是设置一个step数，不会过分地去evaluate，
		// 不会使编译器本末倒置。
		//------------------------------------------------------------------
		unsigned StepsLeft;

		// BottomFrame - The frame in which evaluation started. This must be 
		// initialized after CurrentCall and CallStackDepth.
		//----------------------------nonsense for coding-------------------
		// 从这点来看，编译器相当于内置了一个compile-time的虚拟机，专门来做expression的
		// evaluated.
		//------------------------------------------------------------------
		// CallStackFrame BottomFrame;

		// EvaluatingDecl - This is the declaration whose initializer is being
		// evaluated, if any.
		// 只有变量声明和解包声明会出现常量推导
		//DeclStatement *VD;
		// To Do: 没有实现
		enum EvaluationMode
		{
			/// Evaluate as a constant expression. Stop if we find that the
			/// expression is not a constant expression. -Clang
			EM_ConstantExpression,

			/// Evaluate as a potential constant expression. Keep going if we
			/// hit a construct that we can't evaluate yet(because we don't 
			/// yet know the value of something) but stop if we hit something
			/// that could never be a constant expression.	-Clang
			EM_PotentialConstantExpression,

			/// Fold the expression to a constant. Stop if we hit a side-effect
			/// that we can't model.	-Clang
			EM_ConstantFold,

			/// Evaluate the expression looking for integer overflow and similar
			/// issues. Don't worry about side-effects, and try to visit all
			/// subexpressions.	-Clang
			///---------------------nonsense for coding----------------------
			/// 这里overflow有些鸡肋
			///--------------------------------------------------------------
			EM_EvaluateForOverflow,

			/// Evaluate in any way we know how. Don;t worry about side-effects
			/// that can't be modeled.	-Clang
			EM_IgnoreSideEffects
		} EvalMode;

		/// Are we checking whether the expression is a potential constant 
		/// expression?
		bool checkingPotentialConstantExpression() const
		{
			return EvalMode == EM_PotentialConstantExpression;
		}

		/// Are we checking an expression for overflow?
		// FIXME: We should check for any kind of undefined or suspicious behavior
		// in such constructs, not just overflow. -Clang
		// overflow只是clang符号执行中的很小的一块(好像内置checker没有关于符号执行的)
		bool checkingForOverflow() { return EvalMode == EM_EvaluateForOverflow; }

		// To Do: 对于Compile-time的computation，必须设置步数，否则在规定步数内，没有
		// 计算出constant，则返回false。步数steps暂时硬编码，后面需要用户设置。
		//EvalInfo(Expr::EvalStatus &S, EvaluationMode Mode)
		//	: EvalStatus(S), CE(nullptr), CallStackDepth(0), StepsLeft(32)
		//{}

		//void setEvaluatingDecl(DeclStatement* Decl)
		//{
		//	VD = Decl;
		//}

		bool nextStep();

		// Note that we had a side-effect.
		bool noteSideEffect();
	};
public:

public:
	/// To Do:实现Expr在compile-time的evaluate
	///----------------------------nonsense for coding---------------------
	/// 这里应该仿照Clang实现EvaluateAsRValue()函数，该函数非常强大
	///--------------------------------------------------------------------
	// To Do: 没有实现
	bool EvaluateAsRValue(EvalStatus &Result) const;

	/// To Do: 做简单的常量折叠
	// To Do: 没有实现
	bool EvaluateAsInt(int &Result) const;

	/// EvaluateAsBooleanCondition - Return true if this is a boolean constant 
	/// which we can fold. Using any crazy technique that we want to, even if 
	/// the expression has side-effects.
	// To Do: 没有实现
	bool EvaluateAsBooleanCondition(bool &Result) const;

	// 快速evaluate版本
	// To Do: 没有实现
	//static bool FastEvaluateAsRValue(const Expr *Exp, Expr::EvalStatus &Result,
		//bool &IsConst);

	// 强化的evaluate版本
	// To Do: 没有实现
	//static bool EvaluateAsRValue(const Expr* E, EvalInfo &Result);

	/// Check that this core constant expression is of literal type.
	//static bool CheckLiteralType(const Expr* E);

	// Top level Expr::EvaluateAsRValue method.
	//static bool Evaluate(const Expr *E, EvalInfo &Info);

	/// HasSideEffects - This routine returns true for all those expressions
	/// which have any effect other than producing a value. Example is a function
	/// call, volatile variable read, or throwing an exception. If 
	/// IncludePossiableEffects is false, this call treats certain expressions with
	/// potential side effects (such as function call-like expression, instantiation-
	/// dependent expression, or invocations from a macro) as not having side effects.
	/// To Do:没有实现
	bool HasSideEffects() const;
};

#endif