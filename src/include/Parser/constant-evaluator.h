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
// moses��constant folding��Ҫ�����ں�˵��Ż���������ǿ���Եģ�Ҳ����˵���
// expression���ܽ���constant folding�����ᱨ��moses��constant folding��Clang
// �е� potential constant expression�Ƚ����ơ�
// 
// moses��constant folding��Ҫ��Ϊ�����֣�
// (1) BinaryExpression��constant-folding
//	���磺
//		������һ��
//		var num = 10;
//		...
//		// start can't folding����Ϊ��֪���м��Ƿ��num���и�ֵ��������Ҫ��;�ռ���num�ĸ�ֵ��Ϣ
//		// ���ڸ���
//		var start = num * 13 + 12; 
//		
//		����������
//		const num = 10;
//		...
//		// ��ͬ��������һ���������num��const���ͣ�����ȷ���м�û�ж�����еĸ�ֵ�޸ģ����Կ���
//		// ��start constant-fold��142
//		var start = num * 13 + 12
//		
//		����������
//		// ͬ��flag ����const�����ڼ䣬���Ὣ flag && isGreater() �۵���false���������
//		// isGreater()�����и����õĻ���Ҳ�������constant-folding.
//		var flag = false;
//		if (flag && isGreater()) {} else {}
//
//		�������ģ�
//		// ��isGreater()����û�и����õ�����£����Խ� flag&&isGreater() �۵���false.
//		constant flag = false;
//		if (flag && isGreater()) {} else {}
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
//		���� add(parm1, parm2) < 10 ����constant-folding��false��Ҳ��������һ��
//		��else block��ִ�У�������С��code size�����ٵ�branch����С��ִ�д���
//
//	Note: ����CallExpr��constant-foldingʹ�����������ƣ�
//		��1�� ���������ǿ��Ƶ���
//		��2�� ֻ�е� return ���
//		
//---------------------------------------------------------------------------//
#include "ast.h"
class ConstantEvaluator
{
	/// EvalStatus is a struct with detailed info about an evaluation in progress. -Clang
	/// ���°��Clang�ṩ��һ��EvaluateExpressionVisitorģ�飬�ṩ����ǿ���evaluate.
	/// ����ṹ����Ϊcompile-time��evaluate����ģ�moses�����ṩһ����clangһ��ǿ���
	/// compile-time��evaluate���ơ�����expression evaluated������һ���ǳ���������£�
	/// https://akrzemi1.wordpress.com/2011/05/06/compile-time-computations/ ��ƪ���½�����
	/// compile-time computation.
	/// �Լ�http://clang.llvm.org/docs/InternalsManual.html#constant-folding-in-the-clang-ast
	/// ������ʱдһ��������evaluate����
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
			// ���ڵ�ǰ���ʽ�����ڻ�û�н���evaluate�ķ��ţ������ܱ�evaluate��
			Pending
		};
		/// \brief ������evaluated�ı��ʽ�Ƿ��и����á�
		/// ���磺 (f() && 0) ���Ա��۵���0�����Ǻ���f()�и�����
		bool HasSideEffects;

		EvalStatus() : HasSideEffects(false), result(Result::Pending){}

		// hasSideEffects - Return true if the evaluated expression has side 
		// effects.
		bool hasSideEffects() const
		{
			return HasSideEffects;
		}

		// Note: clang�м��ṩ��һ��ExprResult{}�ṹ�壬���������������ṩ��
		// ���mosesֻ�ṩbool��int���ͣ����Բ���Ҫ�ر��ӵ�ֵ��ʾ�������ڷḻ
		// moses��ʱ�����Ҫ�ṩһ�����Ƶ�ֵ��ʾ��
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
	// To Do: û��ʵ��
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
		// �����Clang��̬���������ƣ���������һ��step����������ֵ�ȥevaluate��
		// ����ʹ��������ĩ���á�
		//------------------------------------------------------------------
		unsigned StepsLeft;

		// BottomFrame - The frame in which evaluation started. This must be 
		// initialized after CurrentCall and CallStackDepth.
		//----------------------------nonsense for coding-------------------
		// ������������������൱��������һ��compile-time���������ר������expression��
		// evaluated.
		//------------------------------------------------------------------
		// CallStackFrame BottomFrame;

		// EvaluatingDecl - This is the declaration whose initializer is being
		// evaluated, if any.
		// ֻ�б��������ͽ����������ֳ����Ƶ�
		//DeclStatement *VD;
		// To Do: û��ʵ��
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
			/// ����overflow��Щ����
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
		// overflowֻ��clang����ִ���еĺ�С��һ��(��������checkerû�й��ڷ���ִ�е�)
		bool checkingForOverflow() { return EvalMode == EM_EvaluateForOverflow; }

		// To Do: ����Compile-time��computation���������ò����������ڹ涨�����ڣ�û��
		// �����constant���򷵻�false������steps��ʱӲ���룬������Ҫ�û����á�
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
	/// To Do:ʵ��Expr��compile-time��evaluate
	///----------------------------nonsense for coding---------------------
	/// ����Ӧ�÷���Clangʵ��EvaluateAsRValue()�������ú����ǳ�ǿ��
	///--------------------------------------------------------------------
	// To Do: û��ʵ��
	bool EvaluateAsRValue(EvalStatus &Result) const;

	/// To Do: ���򵥵ĳ����۵�
	// To Do: û��ʵ��
	bool EvaluateAsInt(int &Result) const;

	/// EvaluateAsBooleanCondition - Return true if this is a boolean constant 
	/// which we can fold. Using any crazy technique that we want to, even if 
	/// the expression has side-effects.
	// To Do: û��ʵ��
	bool EvaluateAsBooleanCondition(bool &Result) const;

	// ����evaluate�汾
	// To Do: û��ʵ��
	//static bool FastEvaluateAsRValue(const Expr *Exp, Expr::EvalStatus &Result,
		//bool &IsConst);

	// ǿ����evaluate�汾
	// To Do: û��ʵ��
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
	/// To Do:û��ʵ��
	bool HasSideEffects() const;
};

#endif