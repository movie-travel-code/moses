//===------------------------------CGCall.cpp-----------------------------===//
//
// This file wrap the information about a call or function definition.
//
//===---------------------------------------------------------------------===//
#include "../../include/IRBuild/IRBuilder.h"
#include "../../include/IRBuild/CodeGenTypes.h"
using namespace compiler::IR;
using namespace compiler::IRBuild;

ValPtr ModuleBuilder::visit(const CallExpr* Call)
{
	return nullptr;
}

/// EmitFunctionPrologue - This function mainly generate prologue code.
///	e.g.	define i32 func(i32 lhs, i32 rhs)
///			{
///				%1 = alloca i32		----------> This is for lhs.
///				%2 = alloca i32		----------> This is for rhs.
///			}
/// Note: Moses's parameter passing will become very complex. We allow the parameter of anonymous
/// type. And we will use reference modle later.
///	e.g.	%struct.1 = {i32, {i1, i32}}
/// 		define i32 func(%struct.1 lhs) {}
///							~~~~~~~~
/// Therefore, we should refer to 'inalloca' attribute in future.
void ModuleBuilder::EmitFunctionPrologue(std::shared_ptr<CGFunctionInfo const> FunInfo, FuncPtr fun)
{
	// If we're using inalloca, all the memory arguments are GEPs off of the last parameter, which
	// is a pointer to the complete memory area.
	/*
	llvm::Value *ArgStruct = nullptr;
	if (IRFunctionArgs.hasInallocaArg())
	{
	ArgStruct = FnArgs[IRFunctionArgs.getInallocaArgNo()];
	assert(ArgStruct->getType() == FI.getArgStruct()->getPointerTo());
	}
	*/

	// Create a pointer value for every parameter declaration. This usually 
	// entails copying one or more IR arguments into an alloca. 
	// e.g.		define i32 @func(i32 %lhs, i32 %rhs)
	//			{
	//				entry:
	//					%retval = alloca i32
	//					%lhs.addr = alloca i32
	//					%rhs.addr = alloca i32			------> Alloca space on callee stack for argument.
	//					~~~~~~~~~~~~~~~~~~~~~~
	//					store i32 %lhs, i32* %lhs.addr
	//					store i32 %rhs, i32* %rhs.addr	------>	And copy the argument value to the 'alloca' space.
	//					~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	//			}
	auto FuncType = fun->getFunctionType();
	for (unsigned i = 0; i < FunInfo->getArgNums(); i++)
	{
		// Create alloca instruciton.
		// To Do: Shit Code!
		/*AllocaInstPtr Alloca = CreateAlloca((*FuncType)[i], (FunInfo->getParmInfo())[i].second);
		CreateStore((*fun)[i], Alloca);*/
		EmitParmDecl(CurFunc->CurFuncDecl->getParmDecl(i).get(), fun->getArg(i));
	}
}

/// EmitFunctionEpilog - Emit the code to return the given temporary.
void ModuleBuilder::EmitFunctionEpilogue()
{
	ValPtr RV = nullptr;
	// Functions with no result always return void.
	if (CurFunc->ReturnValue)
	{
		// e.g.		define @func()
		//			{
		//				entry: 
		//				%retval = alloca i32
		//				...
		//				%0 = load i32* %retval
		//				ret i32 %0
		//			}
		RV = CreateLoad(CurFunc->ReturnValue);
	}
	if (RV)
	{

	}
}

void ModuleBuilder::EmitCall()
{}

void ModuleBuilder::EmitCallArg()
{}

// To Do: Please use 'using' or 'typedef' to simplify std::shared_ptr<compiler::ast::Type> 
CGFunctionInfo::CGFunctionInfo(unsigned NumArgs, 
	std::vector<std::pair<std::shared_ptr<compiler::ast::Type>, std::string>> args, std::shared_ptr<compiler::ast::Type> retty) :
	NumArgs(NumArgs), ArgInfos(args), returnTy(retty)
{}

std::shared_ptr<CGFunctionInfo const> CGFunctionInfo::create(const FunctionDecl *FD)
{
	unsigned num = FD->getParaNum();
	std::vector<std::pair<std::shared_ptr<compiler::ast::Type>, std::string>> args;
	for (auto item : FD->getParms())
	{
		args.push_back({item->getDeclType(), item->getName()});
	}
	auto FunctionInfo = std::make_shared<CGFunctionInfo>(num, args, FD->getReturnType());
	return FunctionInfo;
}

std::shared_ptr<FunctionType> CodeGenTypes::getFunctionType(std::shared_ptr<CGFunctionInfo const> Info)
{
	TyPtr RetTy = ConvertType(Info->getRetTy());
	if (Info->getParmInfo().size() == 1)
		return FunctionType::get(RetTy);
	std::vector<TyPtr> ParmTy;
	for (auto item : Info->getParmInfo())
	{
		ParmTy.push_back(ConvertType(item.first));
	}
	return FunctionType::get(RetTy, ParmTy);
}

/// \brief Generate CGFunctionInfo for FunctionDecl.
/// Note: Moses have no function declaration, so there is no indirect recursion.
std::shared_ptr<CGFunctionInfo const> CodeGenTypes::arrangeFunctionInfo(const FunctionDecl *FD)
{
	std::shared_ptr<CGFunctionInfo const> FI = CGFunctionInfo::create(FD);
	FunctionInfos.insert(FI);
	return FI;
}