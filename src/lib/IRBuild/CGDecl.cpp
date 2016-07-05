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
/// 例如：
///		func add() -> int
///		{
///			var num : int;
///		}
/// 变量 num 需要一条 alloca 指令，分配一块内存。
/// Note: 在moses中，对于function local variable来说，moses没有static类型。
///	变量声明有以下几种形式：
///
/// (1) 内置类型，没有变量初始化
///	 var num : int;		
///  此类VarDecl
///		I.只需要生成一条AllocaInst并注册到SymbolTable.
///	可能的IR指令：%num = alloca i32
///
/// (2) 用户自定义类型，没有变量初始化
///  var num : A;
///	 此类VarDecl
///		I.只需要生成一条AllocaInst并注册到SymbolTable.
///		II.获取用户自定义类型，并注册到CodeGenTypes.h 并注册到MosesIRContext.h.
///	可能的IR指令：%num = alloca %struct.A
///
/// (3) 匿名类型，没有变量初始化
///  var num : {int, {bool, int}, int}; 
///  此类VarDecl
///		I.需要生成一条AllocaInst指令并注册到SymbolTable.
///		II.需要生成 literal struct type(AggregateType). 注册到CodeGenTypes.h 并注册到MosesIRContext.h 
///	可能的IR指令：%num = alloca %struct.1
///
/// (4) 内置类型，变量初始化
///  var num = 10;	 或者    var num = mem;
///	 此类VarDecl
///		I.需要生成AllocaInst并注册到SymbolTable.
///		II.需要为InitEpxr生成代码，并将该结果与AllocaInst连接起来.
///	可能的IR指令：%num = alloca i32				或	%num = alloca i32
///				 store i32 10, i32* %num			%1 = load i32* %mem
///													store i32 %1, i32* %num
///
/// (5) 匿名类型，变量初始化
///  var num = {10, {11, true}, {true, false}};
///  此类VarDecl
///		I.需要生成AllocaInst并注册到SymbolTable.
///		II.需要为InitExpr生成代码，并将该结果与AllocaInst连接起来.
///		III.从ASTContext获取到AnonymousType，注册到CodeGenTypes.h 并注册到MosesIRContext.h
///
/// (6) 用户自定义类型，变量初始化 (此处还有问题)
/// 1).		class A
/// 2).		{
///	3).			var num : int;
///	4).			var flag : bool;
///	5).		};
///	6).		const object : A;
///	7).		object.num = 10;
/// 8).		object.flag = true;
/// 9).		object.num = 10;
/// 此处有两个问题：
///	（一） 用户自定义类型不能列表初始化，例如：const num : A; A = {10, {true, 10}};
/// （二） 用户自定义类型的const属性没有正确处理，如上面第(6)行代码，const属性是附加在
///		  变量object（A）上，还是附加在num和flag上。moses暂时处理的是将const属性附加
///		  在object上。
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

	// (1) 将VarDecl类型转换为IR Type，并创建一条对应的Alloca 指令
	IRTyPtr IRTy = Types.ConvertType(Ty);
	AllocaInstPtr allocInst = CreateAlloca(IRTy, var->getName());

	// (2) 将该条alloca instruction插入到当前VarDecl所在的SymbolTable中
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