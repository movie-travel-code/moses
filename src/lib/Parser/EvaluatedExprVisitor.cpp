//===------------------------EvaluatedExprVisitor.h-----------------------===//
//
// This file defines the EvaluateExprVisitor class.
// 
//===---------------------------------------------------------------------===//
#include "../../include/Parser/EvaluatedExprVisitor.h"
using namespace compiler::ast;

//===-----------------------------------------------------------------------------------===//
// EvaluatedExprVisitorBase's Impletation
bool EvaluatedExprVisitorBase::Evaluate(ExprASTPtr expr, EvalInfo& info)
{
	if (BinaryPtr B = std::dynamic_pointer_cast<BinaryExpr>(expr))
	{
		EvalInfo LhsVal(ValueKind::IntKind, EvaluationMode::EM_ConstantFold);
		EvalInfo RhsVal(ValueKind::IntKind, EvaluationMode::EM_ConstantFold);

		if (!Evaluate(B->getLHS(), LhsVal))
			return false;
		if (!Evaluate(B->getRHS(), RhsVal))
			return false;
		return EvalBinaryExpr(B, info, LhsVal, RhsVal);
	}

	if (UnaryPtr U = std::dynamic_pointer_cast<UnaryExpr>(expr))
	{
		EvalInfo subVal(ValueKind::IntKind, EvaluationMode::EM_ConstantFold);

		if (!Evaluate(U->getSubExpr(), subVal))
			return false;
		return EvalUnaryExpr(U, info, subVal);
	}

	if (MemberExprPtr M = std::dynamic_pointer_cast<MemberExpr>(expr))
	{
		return EvalMemberExpr(M, info);
	}

	if (DeclRefExprPtr DRE = std::dynamic_pointer_cast<DeclRefExpr>(expr))
	{
		return EvalDeclRefExpr(DRE, info);
	}

	if (CallExprPtr CE = std::dynamic_pointer_cast<CallExpr>(expr))
	{
		return EvalCallExpr(CE, info);
	}

	if (BoolLiteralPtr BL = std::dynamic_pointer_cast<BoolLiteral>(expr))
	{
		info.evalstatus.kind = ValueKind::BoolKind;
		info.evalstatus.boolVal = BL->getVal();
		return true;
	}

	if (NumberExprPtr Num = std::dynamic_pointer_cast<NumberExpr>(expr))
	{
		info.evalstatus.kind = ValueKind::IntKind;
		info.evalstatus.intVal = Num->getVal();
		return true;
	}
	return false;
}

void EvaluatedExprVisitorBase::handleEvalCallTail()
{
	std::pair<int, unsigned> bookeepingInfo = ActiveBookingInfo[ActiveBookingInfo.size() - 1];
	ActiveBookingInfo.pop_back();
	for (int i = 0; i < bookeepingInfo.second; i++)
	{
		ActiveStack.pop_back();
	}
}

/// \brief 判断FunctionDecl能否evaluate，并检查是否超出eval call的层数（当前一层）。
ReturnStmtPtr EvaluatedExprVisitorBase::handleEvalCallStart(CallExprPtr CE)
{
	ReturnStmtPtr returnStmt = CE->getFuncDecl()->isEvalCandiateAndGetReturnStmt();

	if (ActiveBookingInfo.size() >= 1)
	{
		return nullptr;
	}
	return returnStmt;
}

/// \brief 这里对CallExpr()进行evaluation.
/// 例如：
/// func add(parm1 : int, parm2 : int) -> int { return parm1 + parm2; }
/// var num = add(10, 20);
/// 上面我们会将 add(10, 20) evaluate成为30.
/// （1） 首先我们会对实参值进行eval，记录下 (name, value) pair，然后应用到函数体中
/// （2） 我们要求函数只能有一条return语句。将（name, value）pair应用到return语句中，进行运算
/// （3） 最终，将得到的结果值作为CallExpr的evaluate得到的constant.
bool EvaluatedExprVisitorBase::EvalCallExpr(CallExprPtr CE, EvalInfo &info)
{
	// (0) 首先判断FunctionDecl能否evaluate，检查是否超出eval call的层数（当前一层）。
	ReturnStmtPtr returnStmt = handleEvalCallStart(CE);
	if (!returnStmt)
	{
		return false;
	}

	ValueKind vk;
	// (1) 对实参值进行evaluate
	unsigned num = CE->getArgsNum();
	for (int i = 0; i < num; i++)
	{
		ExprASTPtr Arg = CE->getArg(i);
		if (Arg->getType()->getKind() == TypeKind::INT)
		{
			vk = ValueKind::IntKind;
		}
		else if (Arg->getType()->getKind() == TypeKind::BOOL)
		{
			vk = ValueKind::BoolKind;
		}
		else
		{
			return false;
		}

		// 对Arg进行evaluate
		EvalInfo info(vk, EvaluationMode::EM_ConstantFold);
		Evaluate(Arg, info);

		// 组成 <name, value> pair存储起来，记录这一层的实参info
		ActiveStack.push_back(std::make_pair(CE->getFuncDecl()->getParmName(i), info));
	}

	// 记录此次的bookkeeping info
	ActiveBookingInfo.push_back(std::make_pair(ActiveBookingInfo.size(), num));

	// (2) 获取CallExpr对应函数体中的ReturnStmt，然后执行运算，得到constant值，然后返回.
	EvalInfo returnInfo(vk, EvaluationMode::EM_ConstantFold);
	ExprASTPtr rexpr = returnStmt->getSubExpr();
	Evaluate(rexpr, returnInfo);

	// (3) 对EvalCall进行收尾处理
	handleEvalCallTail();

	// (4) 对得到的结果进行综合处理
	info = returnInfo;
	return true;
}

/// \brief 用于对DeclRefExpr进行evaluate.
bool EvaluatedExprVisitorBase::EvalDeclRefExpr(DeclRefExprPtr DRE, EvalInfo &info)
{
	auto decl = DRE->getDecl();
	// 检查DeclRefExpr引用的变量是否是const，不是const返回不进行constant-evaluator.
	if (VarDeclPtr VD = std::dynamic_pointer_cast<VarDecl>(decl))
	{
		if (!VD->isConst())
			return false;
		EvalInfo declInfo(info.evalstatus.kind, EvaluationMode::EM_ConstantFold);
		if (!Evaluate(VD->getInitExpr(), declInfo))
		{
			return false;
		}
		info = declInfo;
		return true;
	}
	// 如果是ParmDecl，则检查ParmDecl是否是已经被推导出来了
	else if (ParmDeclPtr PD = std::dynamic_pointer_cast<ParameterDecl>(decl))
	{
		// 检查当前的parmdecl对应的实参，是否已经evaluate出来
		// 例如：
		// func add(lhs : int, rhs : int) -> int 
		// {
		//		return lhs + rhs;
		// }
		// add(10, 20);      <----------------evaluated
		// 我们已经得到了 <lhs, 10> <rhs, 20>，所以evaluate "return lhs + rhs;"
		auto bookkeeping = ActiveBookingInfo.back();
		auto num = bookkeeping.second;
		auto stackSize = ActiveStack.size();
		// ActiveStack 尾部开始检查是否存在parm，不存在，则直接返回。
		// <start, Evalinfo> <lhs, EvalInfo> <rhs, EvalInfo>
		for (int i = stackSize - 1; i >= 0; i--)
		{
			if (ActiveStack[i].first == PD->getParmName())
			{
				info = ActiveStack[i].second;
				return true;
			}
		}
	}	
	return false;;
}

//===-----------------------------------------------------------------------------------===//
// IntExprEvaluator's Impletation
bool IntExprEvaluator::EvalBinaryExpr(BinaryPtr B, EvalInfo &info,
	EvalInfo &lhs, EvalInfo &rhs)
{
	std::string OpName = B->getOpcode();
	if (OpName == "+")
	{
		info.evalstatus.intVal = lhs.evalstatus.intVal + rhs.evalstatus.intVal;
	}
	else if (OpName == "-")
	{
		info.evalstatus.intVal = lhs.evalstatus.intVal - rhs.evalstatus.intVal;
	}
	else if (OpName == "*")
	{
		info.evalstatus.intVal = lhs.evalstatus.intVal * rhs.evalstatus.intVal;
	}
	else if (OpName == "/")
	{
		info.evalstatus.intVal = lhs.evalstatus.intVal / rhs.evalstatus.intVal;
	}
	else if (OpName == "%")
	{
		info.evalstatus.intVal = lhs.evalstatus.intVal % rhs.evalstatus.intVal;
	}
	else
	{
		return false;
	}
	return true;
}

/// \brief 对于整型的单目运算符不存在
bool IntExprEvaluator::EvalUnaryExpr(UnaryPtr U, EvalInfo &info, EvalInfo &subVal)
{
	std::string OpName = U->getOpcode();
	if (OpName == "-")
	{
		info.evalstatus.intVal = 0 - subVal.evalstatus.intVal;
		return true;
	}
	return false;
}

// To Do: 等待实现
bool IntExprEvaluator::EvalMemberExpr(MemberExprPtr ME, EvalInfo &info)
{
	return false;
	if (UDTyPtr UDT = std::dynamic_pointer_cast<UserDefinedType>(ME->getBase()->getType()))
	{
		// UserDefinedType的Member是否是const类型。
		// if (!UDT->getMemberType(ME->getMemberName())->isConst())
			return false;
		// 获取const成员的初始化表达式
		// 例如：
		// class A
		// {
		//		var num : int;
		//		const length : int;
		// }
		// A a;
		// a.num = 10;
		// a.length = 11;	/* const member成员初始化*/
		// 必须能够获取到11这个初始化表达式，该表达式必须存储在a对应成员的初始化表达式中。
	}
	return false;
}

//===-----------------------------------------------------------------------------------===//
// BoolExprEvaluator's Impletation
bool BoolExprEvaluator::EvalBinaryExpr(BinaryPtr B, EvalInfo &info,
	EvalInfo &lhs, EvalInfo &rhs)
{
	std::string OpName = B->getOpcode();
	if (OpName == "||")
	{
		info.evalstatus.boolVal = lhs.evalstatus.boolVal || rhs.evalstatus.boolVal;
		return true;
	}
	else if (OpName == "&&")
	{
		info.evalstatus.boolVal = lhs.evalstatus.boolVal && rhs.evalstatus.boolVal;
		return true;
	}
	else if (OpName == "==")
	{
		/// \brief lhs是bool类型(由于进行过语义分析，所以右侧肯定也是bool类型)
		if (lhs.evalstatus.kind == ValueKind::BoolKind)
		{
			info.evalstatus.boolVal = lhs.evalstatus.boolVal == rhs.evalstatus.boolVal;
		}

		if (lhs.evalstatus.kind == ValueKind::IntKind)
		{
			info.evalstatus.boolVal = lhs.evalstatus.intVal == rhs.evalstatus.intVal;
		}
		return true;
	}
	else if (OpName == "!=")
	{
		/// \brief lhs是bool类型（由于进行过语义分析，所以右侧肯定也是bool类型）
		if (lhs.evalstatus.kind == ValueKind::BoolKind)
		{
			info.evalstatus.boolVal = lhs.evalstatus.boolVal != rhs.evalstatus.boolVal;
		}

		if (lhs.evalstatus.kind == ValueKind::IntKind)
		{
			info.evalstatus.boolVal = lhs.evalstatus.intVal != rhs.evalstatus.intVal;
		}
	}
	else
	{
		return false;
	}
	return true;
}

/// \brief 对于bool类型来说，单目运算符只有!
bool BoolExprEvaluator::EvalUnaryExpr(UnaryPtr U, EvalInfo &info, EvalInfo &subVal)
{
	info.evalstatus.boolVal = !subVal.evalstatus.boolVal;
	return false;
}

// To Do: 过于复杂，咱不实现
bool BoolExprEvaluator::EvalMemberExpr(MemberExprPtr ME, EvalInfo &info)
{
	return false;
}