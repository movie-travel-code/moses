//===--------------------------------CGDecl.cpp---------------------------===//
//
// This contains code to emit Decl nodes.
//
//===---------------------------------------------------------------------===//
#include "../../include/IRBuild/IRBuilder.h"
using namespace compiler::ast;
using namespace compiler::IR;
using namespace compiler::IRBuild;
using namespace compiler::CodeGen;

/// EmitVarDecl - This method handles emission of variable declaration inside
/// a function.  Emit code and set the symbol table entry about this var.
/// ���磺
///		func add() -> int
///		{
///			var num : int;
///		}
/// ���� num ��Ҫһ�� alloca ָ�����һ���ڴ档
/// Note: ��moses�У�����function local variable��˵��mosesû��static���͡�
///	�������������¼�����ʽ��
///
/// (1) �������ͣ�û�б�����ʼ��
///	 var num : int;		
///  ����VarDecl
///		I.ֻ��Ҫ����һ��AllocaInst��ע�ᵽSymbolTable.
///
/// (2) �û��Զ������ͣ�û�б�����ʼ��
///  var num : A;
///	 ����VarDecl
///		I.ֻ��Ҫ����һ��AllocaInst��ע�ᵽSymbolTable.
///		II.��ȡ�û��Զ������ͣ���ע�ᵽCodeGenTypes.h ��ע�ᵽMosesIRContext.h.
///
/// (3) �������ͣ�û�б�����ʼ��
///  var num : {int, {bool, int}, int}; 
///  ����VarDecl
///		I.��Ҫ����һ��AllocaInstָ�ע�ᵽSymbolTable.
///		II.��Ҫ���� literal struct type(AggregateType). ע�ᵽCodeGenTypes.h ��ע�ᵽMosesIRContext.h 
///
/// (4) �������ͣ�������ʼ��
///  var num = 10;	
///	 ����VarDecl
///		I.��Ҫ����AllocaInst��ע�ᵽSymbolTable.
///		II.��ҪΪInitEpxr���ɴ��룬�����ý����AllocaInst��������.
///
/// (5) �������ͣ�������ʼ��
///  var num = {10, {11, true}, {true, false}};
///  ����VarDecl
///		I.��Ҫ����AllocaInst��ע�ᵽSymbolTable.
///		II.��ҪΪInitExpr���ɴ��룬�����ý����AllocaInst��������.
///		III.��ASTContext��ȡ��AnonymousType��ע�ᵽCodeGenTypes.h ��ע�ᵽMosesIRContext.h
///
/// (6) �û��Զ������ͣ�������ʼ�� (�˴���������)
/// 1).		class A
/// 2).		{
///	3).			var num : int;
///	4).			var flag : bool;
///	5).		};
///	6).		const object : A;
///	7).		object.num = 10;
/// 8).		object.flag = true;
/// 9).		object.num = 10;
/// �˴����������⣺
///	��һ�� �û��Զ������Ͳ����б��ʼ�������磺const num : A; A = {10, {true, 10}};
/// ������ �û��Զ������͵�const����û����ȷ�����������(6)�д��룬const�����Ǹ�����
///		  ����object��A���ϣ����Ǹ�����num��flag�ϡ�moses��ʱ������ǽ�const���Ը���
///		  ��object�ϡ�
void ModuleBuilder::EmitLocalVarDecl(VarDeclPtr var)
{
	// �Ժ���������AllocaInst
	ValPtr DeclPtr = nullptr;
}

/// \brief EmitLocalVarAlloca - Emit tha alloca for a local variable.
void ModuleBuilder::EmitLocalVarAlloca(VarDeclPtr var)
{
	ASTTyPtr Ty = var->getDeclType();
	ValPtr DeclPtr;

	// (1) ��VarDecl����ת��ΪIR Type��������һ����Ӧ��Alloca ָ��
	IRTyPtr IRTy = Types.ConvertType(Ty);
	AllocaInstPtr allocInst = CreateAlloca(IRTy, var->getName());

	DeclPtr = allocInst;

	// (2) ������alloca instruction���뵽��ǰVarDecl���ڵ�SymbolTable��
	if (std::shared_ptr<VariableSymbol> varSym = 
		std::dynamic_pointer_cast<VariableSymbol>(CurScope->Resolve(var->getName())))
	{
		varSym->setAllocaInst(allocInst);
	}

	// (3) If this local has an initializer, emit it now.
	ExprASTPtr Init = var->getInitExpr();
	if (Init)
	{
		// LValue lv = 
		ValPtr Loc = DeclPtr;
	}
}

void ModuleBuilder::visit(const VarDecl* VD)
{

}

void ModuleBuilder::visit(const ClassDecl* CD)
{

}

void ModuleBuilder::visit(const FunctionDecl* FD)
{

}

void ModuleBuilder::visit(const UnpackDecl* UD)
{

}