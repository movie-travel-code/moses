//===-----------------------------CGExprAgg.cpp---------------------------===//
//
// This file handle the aggregate expr code generation.
//
//===---------------------------------------------------------------------===//
#include "../../include/IRBuild/IRBuilder.h"
using namespace compiler::IR;
using namespace compiler::IRBuild;
using namespace compiler::CodeGen;

/// \brief EmitAggExpr -  Emit the computation of the specified expression of aggregate type
/// 2016-7-28 Aggregate 直接参与的表达式也就只有 return stmt和赋值，参数传递
/// Aggregate 不参与算数运算（暂时不支持，如果重载运算符的话）。
void ModuleBuilder::EmitAggExpr(const Expr* E, ValPtr DestPtr)
{

}

/// \brief 
void ModuleBuilder::EmitDeclRefExprAgg(const DeclRefExpr *DRE, ValPtr DestPtr)
{
	EmitAggLoadOfLValue(DRE, DestPtr);
}

/// \brief
void ModuleBuilder::EmitMemberExprAgg(const MemberExpr *ME, ValPtr DestPtr)
{

}

/// \brief
void ModuleBuilder::EmitCallExprAgg(const CallExpr* CE, ValPtr DestPtr)
{

}

void ModuleBuilder::EmitAggregateCopy(ValPtr DestPtr, ValPtr SrcPtr, ASTTyPtr Ty)
{
	// Aggregate assignment turns into Intrinsic::ir.memcpy.
	// Get the memcpy intrinsic function.
	std::cout << "memcpy \n";
}

/// ?
void ModuleBuilder::EmitAggLoadOfLValue(const Expr* E, ValPtr DestPtr)
{
	// Get LValue of the 'E', for example DeclRefExpr.
	LValue LV = EmitLValue(E);
	EmitFinalDestCopy(E, LV, DestPtr);
}

/// EmitFinalDestCopy - Perform the final copy to DestPtr( RValue ----> DestPtr ).
void ModuleBuilder::EmitFinalDestCopy(const Expr *E, RValue Src, ValPtr DestPtr)
{
	assert(Src.isAggregate() && "value must be aggregate value!");
	if (!DestPtr)
		return;
	EmitAggregateCopy(DestPtr, Src.getAggregateAddr(), E->getType());
}

/// EmitFinalDestCopy - Perform the final copy to DestPtr( Expr ----> DestPtr ).
void ModuleBuilder::EmitFinalDestCopy(const Expr *E, LValue Src, ValPtr DestPtr)
{
	EmitFinalDestCopy(E, RValue::getAggregate(Src.getAddress()), DestPtr);
}