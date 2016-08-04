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
/// \brief EmitAggExpr -  Emit the computation of the specified expression of aggregate type
/// 2016-7-28 Aggregate 直接参与的表达式也就只有 return stmt和赋值，参数传递
/// Aggregate 不参与算数运算（暂时不支持，如果重载运算符的话）。
void ModuleBuilder::EmitAggExpr(const Expr* E, ValPtr DestPtr)
{
	if (const DeclRefExpr* DRE = dynamic_cast<const DeclRefExpr*>(E))
	{
		EmitAggLoadOfLValue(DRE, DestPtr);
	}

	if (const MemberExpr* ME = dynamic_cast<const MemberExpr*>(E))
	{
		EmitMemberExprAgg(ME, DestPtr);
	}

	if (const CallExpr* CE = dynamic_cast<const CallExpr*>(E))
	{
		EmitCallExprAgg(CE, DestPtr);
	}
}

void ModuleBuilder::EmitDeclRefExprAgg(const DeclRefExpr *DRE, ValPtr DestPtr)
{
	EmitAggLoadOfLValue(DRE, DestPtr);
}

void ModuleBuilder::EmitMemberExprAgg(const MemberExpr *ME, ValPtr DestPtr)
{
	EmitAggLoadOfLValue(ME, DestPtr);
}

void ModuleBuilder::EmitCallExprAgg(const CallExpr* CE, ValPtr DestPtr)
{
	// EmitCallExpr()返回的ValPtr，有三种情况
	// (1) EmitCallExpr() 为Builtin类型，那么返回的就是CallExpr的到值
	//     这一种普通的EmitCallExpr()就可以处理
	// (2) EmitCallExpr() 为AggregateType，但是Coerce到某个BuiltinType，那么返回的是其中的值
	//     这一种比较麻烦，因为面临的状况是：两者的类型不同，外部用于接收返回值得类型是AggregateType
	//     而返回的类型是coerce得到的类型，两者类型不同。
	// (3) EmitCallExpr() 为AggregateType，使用的是sret，那么返回的是TempAlloca指令
	auto rvalue = EmitCallExpr(CE);
	EmitFinalDestCopy(CE, rvalue, DestPtr);
}

void ModuleBuilder::EmitAggregateCopy(ValPtr DestPtr, ValPtr SrcPtr, ASTTyPtr Ty)
{
	assert(Types.ConvertType(Ty)->isAggregateType() && "The Object must have aggregate type.");
	// Aggregate assignment turns into Intrinsic::ir.memcpy.
	// Get the memcpy intrinsic function.
	auto IRMemcpy = Context.getMemcpy();
	std::vector<ValPtr> Args = {DestPtr, SrcPtr};
	auto call = CreateIntrinsic(IRMemcpy, Args);
	print(call);
}

/// ?
ValPtr ModuleBuilder::EmitAggLoadOfLValue(const Expr* E, ValPtr DestPtr)
{
	// Ignore the value.
	if (DestPtr)
	{
		// Get LValue of the 'E', for example DeclRefExpr.
		LValue LV = EmitLValue(E);
		EmitFinalDestCopy(E, LV, DestPtr);
	}	
	return DestPtr;
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