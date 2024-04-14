//===-----------------------------CGExprAgg.cpp---------------------------===//
//
// This file handle the aggregate expr code generation.
//
//===---------------------------------------------------------------------===//
#include "IRBuild/IRBuilder.h"
using namespace IR;
using namespace IRBuild;
using namespace CodeGen;
extern void print(std::shared_ptr<IR::Value> V);
/// \brief EmitAggExpr -  Emit the computation of the specified expression of
/// aggregate type
void ModuleBuilder::EmitAggExpr(const Expr *E, std::shared_ptr<Value> DestPtr) {
  if (const DeclRefExpr *DRE = dynamic_cast<const DeclRefExpr *>(E)) {
    EmitAggLoadOfLValue(DRE, DestPtr);
  }

  if (const MemberExpr *ME = dynamic_cast<const MemberExpr *>(E)) {
    EmitMemberExprAgg(ME, DestPtr);
  }

  if (const CallExpr *CE = dynamic_cast<const CallExpr *>(E)) {
    EmitCallExprAgg(CE, DestPtr);
  }
}

void ModuleBuilder::EmitDeclRefExprAgg(const DeclRefExpr *DRE, std::shared_ptr<Value> DestPtr) {
  EmitAggLoadOfLValue(DRE, DestPtr);
}

void ModuleBuilder::EmitMemberExprAgg(const MemberExpr *ME, std::shared_ptr<Value> DestPtr) {
  EmitAggLoadOfLValue(ME, DestPtr);
}

void ModuleBuilder::EmitCallExprAgg(const CallExpr *CE, std::shared_ptr<Value> DestPtr) {
  auto rvalue = EmitCallExpr(CE);
  EmitFinalDestCopy(CE, rvalue, DestPtr);
}

void ModuleBuilder::EmitAggregateCopy(std::shared_ptr<Value> DestPtr, std::shared_ptr<Value> SrcPtr,
                                      [[maybe_unused]]std::shared_ptr<ASTType> Ty) {
  assert(Types.ConvertType(Ty)->isAggregateType() &&
         "The Object must have aggregate type.");
  // Aggregate assignment turns into Intrinsic::ir.memcpy.
  // Get the memcpy intrinsic function.
  auto IRMemcpy = Context.getMemcpy();
  std::vector<std::shared_ptr<Value>> Args = {DestPtr, SrcPtr};
  auto call = CreateIntrinsic(IRMemcpy, Args);
}

/// ?
std::shared_ptr<Value> ModuleBuilder::EmitAggLoadOfLValue(const Expr *E, std::shared_ptr<Value> DestPtr) {
  // Ignore the value.
  if (DestPtr) {
    // Get LValue of the 'E', for example DeclRefExpr.
    LValue LV = EmitLValue(E);
    EmitFinalDestCopy(E, LV, DestPtr);
  }
  return DestPtr;
}

/// EmitFinalDestCopy - Perform the final copy to DestPtr( RValue ----> DestPtr
/// ).
void ModuleBuilder::EmitFinalDestCopy(const Expr *E, RValue Src,
                                      std::shared_ptr<Value> DestPtr) {
  assert(Src.isAggregate() && "value must be aggregate value!");
  if (!DestPtr)
    return;
  EmitAggregateCopy(DestPtr, Src.getAggregateAddr(), E->getType());
}

/// EmitFinalDestCopy - Perform the final copy to DestPtr( Expr ----> DestPtr ).
void ModuleBuilder::EmitFinalDestCopy(const Expr *E, LValue Src,
                                      std::shared_ptr<Value> DestPtr) {
  EmitFinalDestCopy(E, RValue::getAggregate(Src.getAddress()), DestPtr);
}
