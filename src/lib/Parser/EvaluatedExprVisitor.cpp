//===------------------------EvaluatedExprVisitor.h-----------------------===//
//
// This file defines the EvaluateExprVisitor class.
//
//===---------------------------------------------------------------------===//
#include "../../include/Parser/EvaluatedExprVisitor.h"
using namespace compiler::ast;

//===-----------------------------------------------------------------------------------===//
// EvaluatedExprVisitorBase's Impletation
bool EvaluatedExprVisitorBase::Evaluate(ExprASTPtr expr, EvalInfo &info) {
  if (BinaryPtr B = std::dynamic_pointer_cast<BinaryExpr>(expr)) {
    EvalInfo LhsVal(ValueKind::IntKind, EvaluationMode::EM_ConstantFold);
    EvalInfo RhsVal(ValueKind::IntKind, EvaluationMode::EM_ConstantFold);

    if (!Evaluate(B->getLHS(), LhsVal))
      return false;
    if (!Evaluate(B->getRHS(), RhsVal))
      return false;
    return EvalBinaryExpr(B, info, LhsVal, RhsVal);
  }

  if (UnaryPtr U = std::dynamic_pointer_cast<UnaryExpr>(expr)) {
    EvalInfo subVal(ValueKind::IntKind, EvaluationMode::EM_ConstantFold);

    if (!Evaluate(U->getSubExpr(), subVal))
      return false;
    return EvalUnaryExpr(U, info, subVal);
  }

  if (MemberExprPtr M = std::dynamic_pointer_cast<MemberExpr>(expr)) {
    return EvalMemberExpr(M, info);
  }

  if (DeclRefExprPtr DRE = std::dynamic_pointer_cast<DeclRefExpr>(expr)) {
    return EvalDeclRefExpr(DRE, info);
  }

  if (CallExprPtr CE = std::dynamic_pointer_cast<CallExpr>(expr)) {
    return EvalCallExpr(CE, info);
  }

  if (BoolLiteralPtr BL = std::dynamic_pointer_cast<BoolLiteral>(expr)) {
    info.evalstatus.kind = ValueKind::BoolKind;
    info.evalstatus.boolVal = BL->getVal();
    return true;
  }

  if (NumberExprPtr Num = std::dynamic_pointer_cast<NumberExpr>(expr)) {
    info.evalstatus.kind = ValueKind::IntKind;
    info.evalstatus.intVal = Num->getVal();
    return true;
  }
  return false;
}

void EvaluatedExprVisitorBase::handleEvalCallTail() {
  std::pair<int, unsigned> bookeepingInfo =
      ActiveBookingInfo[ActiveBookingInfo.size() - 1];
  ActiveBookingInfo.pop_back();
  for (unsigned i = 0; i < bookeepingInfo.second; i++) {
    ActiveStack.pop_back();
  }
}

/// \brief �ж�FunctionDecl�ܷ�evaluate��������Ƿ񳬳�eval
/// call�Ĳ�������ǰһ�㣩��
ReturnStmtPtr EvaluatedExprVisitorBase::handleEvalCallStart(CallExprPtr CE) {
  ReturnStmtPtr returnStmt =
      CE->getFuncDecl()->isEvalCandiateAndGetReturnStmt();

  if (ActiveBookingInfo.size() >= 1) {
    return nullptr;
  }
  return returnStmt;
}

/// \brief �����CallExpr()����evaluation.
/// ���磺
/// func add(parm1 : int, parm2 : int) -> int { return parm1 + parm2; }
/// var num = add(10, 20);
/// �������ǻὫ add(10, 20) evaluate��Ϊ30.
/// ��1�� �������ǻ��ʵ��ֵ����eval����¼�� (name, value) pair��Ȼ��Ӧ�õ���������
/// ��2�� ����Ҫ����ֻ����һ��return��䡣����name,
/// value��pairӦ�õ�return����У��������� ��3��
/// ���գ����õ��Ľ��ֵ��ΪCallExpr��evaluate�õ���constant.
bool EvaluatedExprVisitorBase::EvalCallExpr(CallExprPtr CE, EvalInfo &info) {
  // (0) �����ж�FunctionDecl�ܷ�evaluate������Ƿ񳬳�eval
  // call�Ĳ�������ǰһ�㣩��
  ReturnStmtPtr returnStmt = handleEvalCallStart(CE);
  if (!returnStmt) {
    return false;
  }

  ValueKind vk;
  // (1) ��ʵ��ֵ����evaluate
  unsigned num = CE->getArgsNum();
  for (unsigned i = 0; i < num; i++) {
    ExprASTPtr Arg = CE->getArg(i);
    if (Arg->getType()->getKind() == TypeKind::INT) {
      vk = ValueKind::IntKind;
    } else if (Arg->getType()->getKind() == TypeKind::BOOL) {
      vk = ValueKind::BoolKind;
    } else {
      return false;
    }

    // ��Arg����evaluate
    EvalInfo info(vk, EvaluationMode::EM_ConstantFold);
    Evaluate(Arg, info);

    // ��� <name, value> pair�洢��������¼��һ���ʵ��info
    ActiveStack.push_back(
        std::make_pair(CE->getFuncDecl()->getParmName(i), info));
  }

  // ��¼�˴ε�bookkeeping info
  ActiveBookingInfo.push_back(std::make_pair(ActiveBookingInfo.size(), num));

  // (2) ��ȡCallExpr��Ӧ�������е�ReturnStmt��Ȼ��ִ�����㣬�õ�constantֵ��Ȼ�󷵻�.
  EvalInfo returnInfo(vk, EvaluationMode::EM_ConstantFold);
  ExprASTPtr rexpr = returnStmt->getSubExpr();
  Evaluate(rexpr, returnInfo);

  // (3) ��EvalCall������β����
  handleEvalCallTail();

  // (4) �Եõ��Ľ�������ۺϴ���
  info = returnInfo;
  return true;
}

/// \brief ���ڶ�DeclRefExpr����evaluate.
bool EvaluatedExprVisitorBase::EvalDeclRefExpr(DeclRefExprPtr DRE,
                                               EvalInfo &info) {
  auto decl = DRE->getDecl();
  // ���DeclRefExpr���õı����Ƿ���const������const���ز�����constant-evaluator.
  if (VarDeclPtr VD = std::dynamic_pointer_cast<VarDecl>(decl)) {
    if (!VD->isConst())
      return false;
    EvalInfo declInfo(info.evalstatus.kind, EvaluationMode::EM_ConstantFold);
    if (!Evaluate(VD->getInitExpr(), declInfo)) {
      return false;
    }
    info = declInfo;
    return true;
  }
  // �����ParmDecl������ParmDecl�Ƿ����Ѿ����Ƶ�������
  else if (ParmDeclPtr PD = std::dynamic_pointer_cast<ParameterDecl>(decl)) {
    // ��鵱ǰ��parmdecl��Ӧ��ʵ�Σ��Ƿ��Ѿ�evaluate����
    // ���磺
    // func add(lhs : int, rhs : int) -> int
    // {
    //		return lhs + rhs;
    // }
    // add(10, 20);      <----------------evaluated
    // �����Ѿ��õ��� <lhs, 10> <rhs, 20>������evaluate "return lhs + rhs;"
    auto bookkeeping = ActiveBookingInfo.back();
    auto num = bookkeeping.second;
    auto stackSize = ActiveStack.size();
    // ActiveStack β����ʼ����Ƿ����parm�������ڣ���ֱ�ӷ��ء�
    // <start, Evalinfo> <lhs, EvalInfo> <rhs, EvalInfo>
    for (unsigned i = stackSize - 1; i >= 0; i--) {
      if (ActiveStack[i].first == PD->getParmName()) {
        info = ActiveStack[i].second;
        return true;
      }
    }
  }
  return false;
  ;
}

//===-----------------------------------------------------------------------------------===//
// IntExprEvaluator's Impletation
bool IntExprEvaluator::EvalBinaryExpr(BinaryPtr B, EvalInfo &info,
                                      EvalInfo &lhs, EvalInfo &rhs) {
  std::string OpName = B->getOpcode();
  if (OpName == "+") {
    info.evalstatus.intVal = lhs.evalstatus.intVal + rhs.evalstatus.intVal;
  } else if (OpName == "-") {
    info.evalstatus.intVal = lhs.evalstatus.intVal - rhs.evalstatus.intVal;
  } else if (OpName == "*") {
    info.evalstatus.intVal = lhs.evalstatus.intVal * rhs.evalstatus.intVal;
  } else if (OpName == "/") {
    info.evalstatus.intVal = lhs.evalstatus.intVal / rhs.evalstatus.intVal;
  } else if (OpName == "%") {
    info.evalstatus.intVal = lhs.evalstatus.intVal % rhs.evalstatus.intVal;
  } else {
    return false;
  }
  return true;
}

/// \brief �������͵ĵ�Ŀ�����������
bool IntExprEvaluator::EvalUnaryExpr(UnaryPtr U, EvalInfo &info,
                                     EvalInfo &subVal) {
  std::string OpName = U->getOpcode();
  if (OpName == "-") {
    info.evalstatus.intVal = 0 - subVal.evalstatus.intVal;
    return true;
  }
  return false;
}

// To Do: �ȴ�ʵ��
bool IntExprEvaluator::EvalMemberExpr(MemberExprPtr ME, EvalInfo &info) {
  return false;
  if (UDTyPtr UDT = std::dynamic_pointer_cast<UserDefinedType>(
          ME->getBase()->getType())) {
    // UserDefinedType��Member�Ƿ���const���͡�
    // if (!UDT->getMemberType(ME->getMemberName())->isConst())
    return false;
    // ��ȡconst��Ա�ĳ�ʼ�����ʽ
    // ���磺
    // class A
    // {
    //		var num : int;
    //		const length : int;
    // }
    // A a;
    // a.num = 10;
    // a.length = 11;	/* const member��Ա��ʼ��*/
    // �����ܹ���ȡ��11�����ʼ�����ʽ���ñ��ʽ����洢��a��Ӧ��Ա�ĳ�ʼ�����ʽ�С�
  }
  return false;
}

//===-----------------------------------------------------------------------------------===//
// BoolExprEvaluator's Impletation
bool BoolExprEvaluator::EvalBinaryExpr(BinaryPtr B, EvalInfo &info,
                                       EvalInfo &lhs, EvalInfo &rhs) {
  std::string OpName = B->getOpcode();
  if (OpName == "||") {
    info.evalstatus.boolVal = lhs.evalstatus.boolVal || rhs.evalstatus.boolVal;
    return true;
  } else if (OpName == "&&") {
    info.evalstatus.boolVal = lhs.evalstatus.boolVal && rhs.evalstatus.boolVal;
    return true;
  } else if (OpName == "==") {
    /// \brief lhs��bool����(���ڽ��й���������������Ҳ�϶�Ҳ��bool����)
    if (lhs.evalstatus.kind == ValueKind::BoolKind) {
      info.evalstatus.boolVal =
          lhs.evalstatus.boolVal == rhs.evalstatus.boolVal;
    }

    if (lhs.evalstatus.kind == ValueKind::IntKind) {
      info.evalstatus.boolVal = lhs.evalstatus.intVal == rhs.evalstatus.intVal;
    }
    return true;
  } else if (OpName == "!=") {
    /// \brief lhs��bool���ͣ����ڽ��й���������������Ҳ�϶�Ҳ��bool���ͣ�
    if (lhs.evalstatus.kind == ValueKind::BoolKind) {
      info.evalstatus.boolVal =
          lhs.evalstatus.boolVal != rhs.evalstatus.boolVal;
    }

    if (lhs.evalstatus.kind == ValueKind::IntKind) {
      info.evalstatus.boolVal = lhs.evalstatus.intVal != rhs.evalstatus.intVal;
    }
  } else {
    return false;
  }
  return true;
}

/// \brief ����bool������˵����Ŀ�����ֻ��!
bool BoolExprEvaluator::EvalUnaryExpr(UnaryPtr U, EvalInfo &info,
                                      EvalInfo &subVal) {
  info.evalstatus.boolVal = !subVal.evalstatus.boolVal;
  return false;
}

// To Do: ���ڸ��ӣ��۲�ʵ��
bool BoolExprEvaluator::EvalMemberExpr(MemberExprPtr ME, EvalInfo &info) {
  return false;
}