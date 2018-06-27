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
extern void print(std::shared_ptr<compiler::IR::Value> V);
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
void ModuleBuilder::EmitLocalVarDecl(const VarDecl *var)
{
	// (1) Create AllocInst.
	ValPtr DeclPtr = EmitLocalVarAlloca(var);

	// (2) If 'var' have Initialization expression, emit it.
	// Note: We only handle the built-in type.
	if (auto init = var->getInitExpr())
	{
		auto ty = Types.ConvertType(init->getType());
		if (ty->isAggregateType())
		{
			EmitAggExpr(init.get(), DeclPtr);
		}
		else
		{
			ValPtr V = EmitScalarExpr(init.get());
			EmitStoreOfScalar(V, DeclPtr);
		}
	}
}

/// \brief EmitLocalVarAlloca - Emit tha alloca for a local variable.
AllocaInstPtr ModuleBuilder::EmitLocalVarAlloca(const VarDecl *var)
{
	ASTTyPtr Ty = var->getDeclType();

	// (1) ��VarDecl����ת��ΪIR Type��������һ����Ӧ��Alloca ָ��
	IRTyPtr IRTy = Types.ConvertType(Ty);
	AllocaInstPtr allocInst = CreateAlloca(IRTy, LocalInstNamePrefix + var->getName() + ".addr");

	print(allocInst);

	// (2) ������alloca instruction���뵽��ǰVarDecl���ڵ�SymbolTable��
	if (std::shared_ptr<VariableSymbol> varSym =
			std::dynamic_pointer_cast<VariableSymbol>(CurScope->Resolve(var->getName())))
	{
		assert(!varSym->getAllocaInst() && "Decl's alloca inst already exists.");
		varSym->setAllocaInst(allocInst);
	}
	return allocInst;
}

/// EmitParmDecl - Emit an alloca (or GlobalValue depending on target)
/// for the specified parameter and set up LocalDeclMap.
/// (1) Direct - Alloca Instruction, store.
/// (2) Indirect - Aggregate, DeclPtr = Arg;
/// (3) Ignore.
void ModuleBuilder::EmitParmDecl(const VarDecl *VD, ValPtr Arg)
{
	ValPtr DeclPtr;
	// A fixed sized single-value variable becomes an alloca in the entry block.
	IRTyPtr Ty = Types.ConvertType(VD->getDeclType());

	std::string Name = VD->getName();
	if (Ty->isSingleValueType())
	{
		// e.g. define func(i32 %lhs, i32 %rhs)
		//		{
		//			%lhs.addr = alloca i32
		//		}
		Name = LocalInstNamePrefix + Name + ".addr";
		DeclPtr = CreateAlloca(Ty);
		print(DeclPtr);

		// Store the intial value into the alloca.
		EmitStoreOfScalar(Arg, DeclPtr);
	}
	else
	{
		// otherwise, if this is an aggregate, just use the input pointer.
		DeclPtr = Arg;
	}
	Arg->setName(Name);

	// (1) Get the ParmDecl's SymbolTable Entry.
	auto SymEntry = CurScope->Resolve(VD->getName());
	assert(SymEntry != nullptr && "Parameter declaration doesn't exists.");
	if (std::shared_ptr<ParmDeclSymbol> sym = std::dynamic_pointer_cast<ParmDeclSymbol>(SymEntry))
	{
		assert((sym->getAllocaInst() == nullptr) && "Symbol's alloca instruciton already exists.");
		sym->setAllocaInst(DeclPtr);
	}
}

void ModuleBuilder::EmitScalarInit(const Expr *init, const VarDecl *D, LValue lvalue)
{
}

//===---------------------------------------------------------------------===//
// Generate code for function declaration.
//===---------------------------------------------------------------------===//
void ModuleBuilder::EmitFunctionDecl(const FunctionDecl *FD)
{
	// generate function info.
	std::shared_ptr<CGFunctionInfo const> FI = Types.arrangeFunctionInfo(FD);

	auto TypeAndName = Types.getFunctionType(FD, FI);

	CurFunc->CGFnInfo = FI;
	CurFunc->CurFuncDecl = const_cast<FunctionDecl *>(FD);
	CurFunc->FnRetTy = Types.ConvertType(FD->getReturnType());
	FuncPtr func = Function::create(TypeAndName.first, FD->getFDName(), TypeAndName.second);
	CurFunc->CurFn = func;
	IRs.push_back(func);
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
	FuncSym->setFuncAddr(func);
	CurScope = FuncSym->getScope();

	StartFunction(FI, func);

	// (2) Emit the function body.
	EmitFunctionBody(FD->getCompoundBody());

	// (3) Finish function.
	FinishFunction();
	CurFunc->CurFn = nullptr;

	print(func);

	// (4) Switch the scope back.
	CurScope = CurScope->getParent();
}

/// \brief Handle the start of function.
void ModuleBuilder::StartFunction(std::shared_ptr<CGFunctionInfo const> FnInfo, FuncPtr Fn)
{
	EntryBlock = CreateBasicBlock("entry", Fn);
	// EmitBlock(EntryBB);
	Fn->addBB(EntryBlock);
	SetInsertPoint(EntryBlock);
	AllocaInsertPoint = InsertPoint;
	isAllocaInsertPointSetByNormalInsert = false;
	TempCounter = 0;

	CurFunc->ReturnBlock = CreateBasicBlock("return", CurFunc->CurFn);
	auto RetTy = FnInfo->getReturnInfo()->getType();
	if (RetTy->getKind() == TypeKind::VOID)
	{
		// Void type; nothing to return
		CurFunc->ReturnValue = nullptr;
	}
	else
	{
		CurFunc->ReturnValue = CreateAlloca(Types.ConvertType(RetTy), "%retval");
		print(CurFunc->ReturnValue);
	}
	EmitFunctionPrologue(FnInfo, Fn);
}

void ModuleBuilder::FinishFunction()
{
	EmitReturnBlock();
	EmitFunctionEpilogue(CurFunc->CGFnInfo);
}

ValPtr ModuleBuilder::visit(const VarDecl *VD)
{
	EmitLocalVarDecl(VD);
	return nullptr;
}

/// \brief Handle function declaration.
ValPtr ModuleBuilder::visit(const FunctionDecl *FD)
{
	// (1) switch the scope and save the context-info.
	// e.g.		var num = 10;							----> Old Scope.
	//			func add(lhs:int, rhs:int) -> int		----> New Scope.
	//			{
	//				...
	//			}
	auto FunSym = CurScope->Resolve(FD->getFDName());
	assert(FunSym != nullptr && "Function doesn't exists.");
	SaveTopLevelCtxInfo();

	// (2) Emit code for function.
	auto CurAllocaInsertPoint = AllocaInsertPoint;
	EmitFunctionDecl(FD);

	// (3) Switch CurBB back;
	RestoreTopLevelCtxInfo();

	AllocaInsertPoint = CurAllocaInsertPoint;
	return nullptr;
}

ValPtr ModuleBuilder::visit(const UnpackDecl *UD)
{
	return nullptr;
}