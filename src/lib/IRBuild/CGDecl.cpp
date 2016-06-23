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
///
/// (2) 用户自定义类型，没有变量初始化
///  var num : A;
///	 此类VarDecl
///		I.只需要生成一条AllocaInst并注册到SymbolTable.
///		II.获取用户自定义类型，并注册到CodeGenTypes.h 并注册到MosesIRContext.h.
///
/// (3) 匿名类型，没有变量初始化
///  var num : {int, {bool, int}, int}; 
///  此类VarDecl
///		I.需要生成一条AllocaInst指令并注册到SymbolTable.
///		II.需要生成 literal struct type(AggregateType). 注册到CodeGenTypes.h 并注册到MosesIRContext.h 
///
/// (4) 内置类型，变量初始化
///  var num = 10;	
///	 此类VarDecl
///		I.需要生成AllocaInst并注册到SymbolTable.
///		II.需要为InitEpxr生成代码，并将该结果与AllocaInst连接起来.
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
void ModuleBuilder::EmitLocalVarDecl(VarDeclPtr var)
{
	// 稍后用来保存AllocaInst
	ValPtr DeclPtr = nullptr;
}

/// \brief EmitLocalVarAlloca - Emit tha alloca for a local variable.
void ModuleBuilder::EmitLocalVarAlloca(VarDeclPtr var)
{
	ASTTyPtr Ty = var->getDeclType();
	ValPtr DeclPtr;

	// (1) 将VarDecl类型转换为IR Type，并创建一条对应的Alloca 指令
	IRTyPtr IRTy = Types.ConvertType(Ty);
	AllocaInstPtr allocInst = CreateAlloca(IRTy, var->getName());

	DeclPtr = allocInst;

	// (2) 将该条alloca instruction插入到当前VarDecl所在的SymbolTable中
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