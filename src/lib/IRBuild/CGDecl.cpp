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
///	���ܵ�IRָ�%num = alloca i32
///
/// (2) �û��Զ������ͣ�û�б�����ʼ��
///  var num : A;
///	 ����VarDecl
///		I.ֻ��Ҫ����һ��AllocaInst��ע�ᵽSymbolTable.
///		II.��ȡ�û��Զ������ͣ���ע�ᵽCodeGenTypes.h ��ע�ᵽMosesIRContext.h.
///	���ܵ�IRָ�%num = alloca %struct.A
///
/// (3) �������ͣ�û�б�����ʼ��
///  var num : {int, {bool, int}, int}; 
///  ����VarDecl
///		I.��Ҫ����һ��AllocaInstָ�ע�ᵽSymbolTable.
///		II.��Ҫ���� literal struct type(AggregateType). ע�ᵽCodeGenTypes.h ��ע�ᵽMosesIRContext.h 
///	���ܵ�IRָ�%num = alloca %struct.1
///
/// (4) �������ͣ�������ʼ��
///  var num = 10;	 ����    var num = mem;
///	 ����VarDecl
///		I.��Ҫ����AllocaInst��ע�ᵽSymbolTable.
///		II.��ҪΪInitEpxr���ɴ��룬�����ý����AllocaInst��������.
///	���ܵ�IRָ�%num = alloca i32				��	%num = alloca i32
///				 store i32 10, i32* %num			%1 = load i32* %mem
///													store i32 %1, i32* %num
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
void ModuleBuilder::EmitLocalVarDecl(const VarDecl* var)
{
	// (1) Create AllocInst.
	ValPtr DeclPtr = EmitLocalVarAlloca(var);

	// (2) If 'var' have Initialization expression, emit it.
	// Note: We only handle the built-in type.
	if (auto init = var->getInitExpr())
	{
		ValPtr V = EmitScalarExpr(init.get());
		EmitStoreOfScalar(V, DeclPtr);
	}
	// (3)
}

/// \brief EmitLocalVarAlloca - Emit tha alloca for a local variable.
AllocaInstPtr ModuleBuilder::EmitLocalVarAlloca(const VarDecl* var)
{
	ASTTyPtr Ty = var->getDeclType();

	// (1) ��VarDecl����ת��ΪIR Type��������һ����Ӧ��Alloca ָ��
	IRTyPtr IRTy = Types.ConvertType(Ty);
	AllocaInstPtr allocInst = CreateAlloca(IRTy, var->getName());

	// (2) ������alloca instruction���뵽��ǰVarDecl���ڵ�SymbolTable��
	if (std::shared_ptr<VariableSymbol> varSym = 
		std::dynamic_pointer_cast<VariableSymbol>(CurScope->Resolve(var->getName())))
	{
		varSym->setAllocaInst(allocInst);
	}
	return allocInst;
}

/// EmitParmDecl - Emit an alloca (or GlobalValue depending on target)
/// for the specified parameter and set up LocalDeclMap.
void ModuleBuilder::EmitParmDecl(const VarDecl* VD, ValPtr Arg)
{
	AllocaInstPtr DeclPtr;
	// A fixed sized single-value variable becomes an alloca in the entry block.
	IRTyPtr Ty = Types.ConvertType(VD->getDeclType());
	std::string Name = VD->getName();
	// e.g. define func(i32 %lhs, i32 %rhs)
	//		{
	//			%lhs.addr = alloca i32
	//		}
	Name += ".addr";
	DeclPtr = CreateAlloca(Ty, Name);

	// Store the intial value into the alloca.
	EmitStoreOfScalar(Arg, DeclPtr);

	// (1) Get the ParmDecl's SymbolTable Entry.
	auto SymEntry = CurScope->Resolve(VD->getName());
	assert(SymEntry != nullptr && "Parameter declaration doesn't exists.");
	if (std::shared_ptr<ParmDeclSymbol> sym = std::dynamic_pointer_cast<ParmDeclSymbol>(SymEntry))
	{
		assert((sym->getAllocaInst() == nullptr) && "Symbol's alloca instruciton already exists.");
		sym->setAllocaInst(DeclPtr);
	}
	// Otherwise, if this is an aggregate, just use the input pointer.
	// DeclPtr = Arg;
}


void ModuleBuilder::EmitScalarInit(const Expr* init, const VarDecl* D, LValue lvalue)
{

}

//===---------------------------------------------------------------------===//
// Generate code for function declaration.
//===---------------------------------------------------------------------===//
void ModuleBuilder::EmitFunctionDecl(const FunctionDecl* FD)
{
	// generate function info.
	std::shared_ptr<CGFunctionInfo const> FI = Types.arrangeFunctionInfo(FD);
	std::shared_ptr<FunctionType> Ty = Types.getFunctionType(FI);

	CurFunc->CurFnInfo = FI;
	CurFunc->CurFuncDecl = const_cast<FunctionDecl*>(FD);
	CurFunc->FnRetTy = Types.ConvertType(FD->getReturnType());
	FuncPtr func = Function::create(Ty, FD->getFDName());

	// (1) Start function.
	// Note: we need switch the scope.
	//	e.g.	var num = 10;							----> Old Scope.
	//			func add(lhs:int, rhs : int) -> int		----> New Scope.
	//			{
	//				...
	//			}
	auto Sym = CurScope->Resolve(FD->getFDName());
	assert(Sym != nullptr && "Funciton symbol can't be null.");
	std::shared_ptr<FunctionSymbol> FuncSym = std::dynamic_pointer_cast<FunctionSymbol>(Sym);
	assert(FuncSym != nullptr && "Funciton symbol can't be null.");
	CurScope = FuncSym->getScope();

	StartFunction(FI, func);

	// (2) Function body.	
	EmitFunctionBody(FD->getCompoundBody());

	// (3) Finish function.
	FinishFunction();

	// (4) Switch the scope back.
	CurScope = CurScope->getParent();
}

/// \brief Handle the start of function.
void ModuleBuilder::StartFunction(std::shared_ptr<CGFunctionInfo const> FnInfo, FuncPtr Fn)
{
	BBPtr EntryBB = CreateBasicBlock("entry", CurFunc->CurFn);
	SetInsertPoint(EntryBB);
	AllocaInsertPoint = InsertPoint;

	CurFunc->ReturnBlock = CreateBasicBlock("return", CurFunc->CurFn);
	
	// Set the argument's name and argument's type here.
	for (unsigned i = 0; i < FnInfo->getArgNums(); i++)
	{
		Fn->setArgumentInfo(i, FnInfo->getParm(i).second);
	}

	if (FnInfo->getRetTy()->getKind() == TypeKind::VOID)
	{
		// Void type; nothing to return
		CurFunc->ReturnValue = nullptr;
		// e.g. func print() -> void
		//		{
		//			if (num > 10)
		//			{
		//				return;
		//			}
		//			num ++;
		//			~~~~~~~			--------> implicit return stmt, increase NumReturnExprs.
		//		}
		/*if (!CurFunc->CurFuncDecl->endsWithReturn())
			CurFunc->NumReturnExprs++;*/
	}
	else
	{
		CurFunc->ReturnValue = CreateAlloca(Fn->getReturnType(), "retval");
	}

	EmitFunctionPrologue(FnInfo, Fn);
}

void ModuleBuilder::FinishFunction()
{
	// Emit function epilog(to return).
	EmitReturnBlock();
}

ValPtr ModuleBuilder::visit(const VarDecl* VD)
{
	EmitLocalVarDecl(VD);
	return nullptr;
}

/// \brief 
ValPtr ModuleBuilder::visit(const ClassDecl* CD)
{
	return nullptr;
}

/// \brief Handle function declaration.
ValPtr ModuleBuilder::visit(const FunctionDecl* FD)
{
	// (1) switch the scope.
	// e.g.		var num = 10;							----> Old Scope.
	//			func add(lhs:int, rhs:int) -> int		----> New Scope.
	//			{
	//				...
	//			}
	auto FunSym = CurScope->Resolve(FD->getFDName());
	assert(FunSym != nullptr && "Function doesn't exists.");

	// (2) Emit code for function.
	EmitFunctionDecl(FD);

	return nullptr;
}

ValPtr ModuleBuilder::visit(const UnpackDecl* UD)
{
	return nullptr;
}