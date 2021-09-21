//===------------------------EvaluatedExprVisitor.h-----------------------===//
//
// This file defines the EvaluateExprVisitor class.
//
//===---------------------------------------------------------------------===//
#include "Parser/EvaluatedExprVisitor.h"
using namespace ast;

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

ReturnStmtPtr EvaluatedExprVisitorBase::handleEvalCallStart(CallExprPtr CE) {
  ReturnStmtPtr returnStmt =
      CE->getFuncDecl()->isEvalCandiateAndGetReturnStmt();

  if (ActiveBookingInfo.size() >= 1) {
    return nullptr;
  }
  return returnStmt;
}

/// func add(parm1 : int, parm2 : int) -> int { return parm1 + parm2; }
/// var num = add(10, 20);
bool EvaluatedExprVisitorBase::EvalCallExpr(CallExprPtr CE, EvalInfo &info) {
  ReturnStmtPtr returnStmt = handleEvalCallStart(CE);
  if (!returnStmt) {
    return false;
  }

  ValueKind vk;

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

    EvalInfo info(vk, EvaluationMode::EM_ConstantFold);
    Evaluate(Arg, info);

    ActiveStack.push_back(
        std::make_pair(CE->getFuncDecl()->getParmName(i), info));
  }

  ActiveBookingInfo.push_back(std::make_pair(ActiveBookingInfo.size(), num));

  EvalInfo returnInfo(vk, EvaluationMode::EM_ConstantFold);
  ExprASTPtr rexpr = returnStmt->getSubExpr();
  Evaluate(rexpr, returnInfo);

  handleEvalCallTail();

  info = returnInfo;
  return true;
}

bool EvaluatedExprVisitorBase::EvalDeclRefExpr(DeclRefExprPtr DRE,
                                               EvalInfo &info) {
  auto decl = DRE->getDecl();
  if (VarDeclPtr VD = std::dynamic_pointer_cast<VarDecl>(decl)) {
    if (!VD->isConst())
      return false;
    EvalInfo declInfo(info.evalstatus.kind, EvaluationMode::EM_ConstantFold);
    if (!Evaluate(VD->getInitExpr(), declInfo)) {
      return false;
    }
    info = declInfo;
    return true;
  } else if (ParmDeclPtr PD = std::dynamic_pointer_cast<ParameterDecl>(decl)) {
    // func add(lhs : int, rhs : int) -> int
    // {
    //		return lhs + rhs;
    // }
    // add(10, 20);      <----------------evaluated
    // auto bookkeeping = ActiveBookingInfo.back();
    // auto num = bookkeeping.second;
    auto stackSize = ActiveStack.size();
    // ActiveStack
    // <start, Evalinfo> <lhs, EvalInfo> <rhs, EvalInfo>
    for (int i = stackSize - 1; i >= 0; i--) {
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

bool IntExprEvaluator::EvalUnaryExpr(UnaryPtr U, EvalInfo &info,
                                     EvalInfo &subVal) {
  std::string OpName = U->getOpcode();
  if (OpName == "-") {
    info.evalstatus.intVal = 0 - subVal.evalstatus.intVal;
    return true;
  }
  return false;
}

bool IntExprEvaluator::EvalMemberExpr(MemberExprPtr ME,
                                      [[maybe_unused]] EvalInfo &info) {
  return false;
  if (UDTyPtr UDT = std::dynamic_pointer_cast<UserDefinedType>(
          ME->getBase()->getType())) {
    // UserDefinedType
    // if (!UDT->getMemberType(ME->getMemberName())->isConst())
    return false;
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
    if (lhs.evalstatus.kind == ValueKind::BoolKind) {
      info.evalstatus.boolVal =
          lhs.evalstatus.boolVal == rhs.evalstatus.boolVal;
    }

    if (lhs.evalstatus.kind == ValueKind::IntKind) {
      info.evalstatus.boolVal = lhs.evalstatus.intVal == rhs.evalstatus.intVal;
    }
    return true;
  } else if (OpName == "!=") {
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

bool BoolExprEvaluator::EvalUnaryExpr([[maybe_unused]] UnaryPtr U,
                                      [[maybe_unused]] EvalInfo &info,
                                      EvalInfo &subVal) {
  info.evalstatus.boolVal = !subVal.evalstatus.boolVal;
  return false;
}

bool BoolExprEvaluator::EvalMemberExpr([[maybe_unused]] MemberExprPtr ME,
                                       [[maybe_unused]] EvalInfo &info) {
  return false;
}