//===-----------------------------CGExprAgg.cpp---------------------------===//
//
// This file handle the aggregate expr code generation.
//
//===---------------------------------------------------------------------===//
#include "../../include/IRBuild/IRBuilder.h"
using namespace compiler::IR;
using namespace compiler::IRBuild;
using namespace compiler::CodeGen;
extern void print(std::shared_ptr<compiler::IR::Value> V);
/// \brief EmitAggExpr -  Emit the computation of the specified expression of
/// aggregate type 2016-7-28 Aggregate ֱ�Ӳ���ı��ʽҲ��ֻ�� return
/// stmt�͸�ֵ���������� Aggregate
/// �������������㣨��ʱ��֧�֣��������������Ļ�����
void ModuleBuilder::EmitAggExpr(const Expr *E, ValPtr DestPtr) {
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

void ModuleBuilder::EmitDeclRefExprAgg(const DeclRefExpr *DRE, ValPtr DestPtr) {
  EmitAggLoadOfLValue(DRE, DestPtr);
}

void ModuleBuilder::EmitMemberExprAgg(const MemberExpr *ME, ValPtr DestPtr) {
  EmitAggLoadOfLValue(ME, DestPtr);
}

void ModuleBuilder::EmitCallExprAgg(const CallExpr *CE, ValPtr DestPtr) {
  // EmitCallExpr()���ص�ValPtr�����������
  // (1) EmitCallExpr() ΪBuiltin���ͣ���ô���صľ���CallExpr�ĵ�ֵ
  //     ��һ����ͨ��EmitCallExpr()�Ϳ��Դ���
  // (2) EmitCallExpr()
  // ΪAggregateType������Coerce��ĳ��BuiltinType����ô���ص������е�ֵ
  //     ��һ�ֱȽ��鷳����Ϊ���ٵ�״���ǣ����ߵ����Ͳ�ͬ���ⲿ���ڽ��շ���ֵ��������AggregateType
  //     �����ص�������coerce�õ������ͣ��������Ͳ�ͬ��
  // (3) EmitCallExpr() ΪAggregateType��ʹ�õ���sret����ô���ص���TempAllocaָ��
  auto rvalue = EmitCallExpr(CE);
  EmitFinalDestCopy(CE, rvalue, DestPtr);
}

void ModuleBuilder::EmitAggregateCopy(ValPtr DestPtr, ValPtr SrcPtr,
                                      ASTTyPtr Ty) {
  assert(Types.ConvertType(Ty)->isAggregateType() &&
         "The Object must have aggregate type.");
  // Aggregate assignment turns into Intrinsic::ir.memcpy.
  // Get the memcpy intrinsic function.
  auto IRMemcpy = Context.getMemcpy();
  std::vector<ValPtr> Args = {DestPtr, SrcPtr};
  auto call = CreateIntrinsic(IRMemcpy, Args);
  print(call);
}

/// ?
ValPtr ModuleBuilder::EmitAggLoadOfLValue(const Expr *E, ValPtr DestPtr) {
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
                                      ValPtr DestPtr) {
  assert(Src.isAggregate() && "value must be aggregate value!");
  if (!DestPtr)
    return;
  EmitAggregateCopy(DestPtr, Src.getAggregateAddr(), E->getType());
}

/// EmitFinalDestCopy - Perform the final copy to DestPtr( Expr ----> DestPtr ).
void ModuleBuilder::EmitFinalDestCopy(const Expr *E, LValue Src,
                                      ValPtr DestPtr) {
  EmitFinalDestCopy(E, RValue::getAggregate(Src.getAddress()), DestPtr);
}