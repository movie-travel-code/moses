//===------------------------------CGCall.cpp-----------------------------===//
//
// This file wrap the information about a call or function definition.
//
//===---------------------------------------------------------------------===//
#include "../../include/IRBuild/IRBuilder.h"
#include "../../include/IRBuild/CodeGenTypes.h"
using namespace compiler::IR;
using namespace compiler::IRBuild;

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

/// \brief EmitCall - Emit code for CallExpr.
ValPtr ModuleBuilder::EmitCall(ValPtr FuncAddr, const std::vector<ExprASTPtr> &ArgExprs)
{
	CallArgList Args;
	// (1) EmitCallArgs().
	EmitCallArgs(Args, ArgExprs);

	// (2) EmitCall() -> EmitCallArgs.
	return EmitCall(FuncAddr, Args);
}

/// \brief
ValPtr ModuleBuilder::EmitCall(ValPtr FuncAddr, CallArgList CallArgs)
{
	std::vector<ValPtr> Args;
	for (auto item : CallArgs)
	{
		Args.push_back(item.first);
	}
	return CreateCall(FuncAddr, Args);
}

// EmitCallArgs - Emit call arguments for a function.
void ModuleBuilder::EmitCallArgs(CallArgList &CallArgs, const std::vector<ExprASTPtr> &ArgExprs)
{
	for (auto item : ArgExprs)
	{
		ValPtr V = EmitScalarExpr(item.get());
		CallArgs.push_back({V, item->getType()});
	}
}

/// EmitCallArg - Emit a single call argument.
void ModuleBuilder::EmitCallArg(const Expr* E, compiler::IRBuild::ASTTyPtr ArgType)
{}

//===--------------------------------------------------------------------------===//
// Implements the ArgABIInfo
std::shared_ptr<ArgABIInfo> ArgABIInfo::Create(ASTTyPtr type, Kind kind)
{
	return std::make_shared<ArgABIInfo>(type, kind);
}

//===--------------------------------------------------------------------------===//
// Implements the CGFunctionInfo below.
CGFunctionInfo::CGFunctionInfo(std::vector<ASTTyPtr> ArgsTy, ASTTyPtr RetTy)
{
	// Generate the ArgABIInfo.
	// (1) Create the ArgABIInfo for Return Type.
	ReturnInfo = classifyReturnTye(RetTy);

	// (2) Create the ArgABIInfos for Args.
	for (auto item : ArgsTy)
	{
		ArgInfos.push_back(classifyArgumentType(item));
	}
}

/// \brief classifyReturnType - compute the ArgABIInfo for ReturnType.
///	Rule:	int/bool						----> Direct
///			void							----> Ignore
///			struct{int/bool}				----> Direct(coerce to int)
///			struct{int/bool, int/bool}		----> InDirect(sret hidden pointer)
AAIPtr CGFunctionInfo::classifyReturnTye(ASTTyPtr RetTy)
{
	if (RetTy->getKind() == TypeKind::VOID)
		return std::make_shared<ArgABIInfo>(RetTy, ArgABIInfo::Kind::Ignore);

	// small structures which are register sized are generally returned
	// in register.
	if (RetTy->size() <= 32)
		return std::make_shared<ArgABIInfo>(RetTy, ArgABIInfo::Kind::Direct, IR::Type::getIntType());
	return std::make_shared<ArgABIInfo>(RetTy, ArgABIInfo::Kind::InDirect);
}

/// \brief classifyArgumentType - compute the ArgABIInfo for ArgumentType.
/// Rule:	int/bool						----> Direct
///			void							----> Ignore
///			struct{int/bool}				----> Direct(coerce to int-i32)
///			struct{int/bool, int/bool}		----> Direct(flatten int, int)
///			struct{int/bool, int/bool,,,}	----> Indirect(hidden pointer)
AAIPtr CGFunctionInfo::classifyArgumentType(ASTTyPtr ArgTy)
{
	if (ArgTy->getKind() == TypeKind::VOID)
		return std::make_shared<ArgABIInfo>(ArgTy, ArgABIInfo::Kind::Ignore);

	// small structures which are register sized are generally returned
	// in register or flatten as two registers.
	if (ArgTy->size() <= 32)
	{
		// case 1: class A { var m:bool; };
		// case 2: class A { var m:bool; }; class B{ var m:A; };
		// case 3: var num = {{{int}}}
		// To Do: We need to strip off the '{' and '}' to get the core type.
		return std::make_shared<ArgABIInfo>(ArgTy, ArgABIInfo::Kind::Direct, IR::Type::getIntType());
	}
		

	if (ArgTy->size() <= 64)
		return std::make_shared<ArgABIInfo>(ArgTy, ArgABIInfo::Direct, nullptr, false);
}

std::shared_ptr<CGFunctionInfo const> CGFunctionInfo::create(const FunctionDecl *FD)
{
	std::vector<ASTTyPtr> ArgsTy;
	for (auto item : FD->getParms())
	{
		ArgsTy.push_back(item->getDeclType());
	}
	return std::make_shared<CGFunctionInfo>(ArgsTy, FD->getReturnType());
}

namespace
{
	/// Encapsulates information about the way function from CGFunctionInfo should
	/// be passed to actual IR function.
	/// e.g.	[parm1, parm2, ..., parmn] -> ret
	///						||
	///					    \/
	///			[sret ,type1, coerce-to-type2, ..., {flatten.1, flatten.2}]
	///	ASTToIRMapping is the key to generate function.
	class ASTToIRMapping
	{
		unsigned TotalIRArgs;
		bool HasSRet;
		/// Arguments of IR function correspoding to single AST argument.
		struct IRArgs
		{
			// Argument is expanded to IR arguments at positions.
			// [FirstArgIndex, FirstArgIndex + NumberOfArgs).
			unsigned FirstArgIndex;
			unsigned NumberOfArgs;

			IRArgs(unsigned first, unsigned length) : FirstArgIndex(first), 
				NumberOfArgs(length - 1) 
			{}
		};
		std::vector<IRArgs> ArgInfo;
	public:
		ASTToIRMapping(const CGFunctionInfo &FI) : TotalIRArgs(0), HasSRet(false)
		{
			construct(FI);
		}
		bool hasSRetArg() const { HasSRet; }
		unsigned totalIRArgs() const { return TotalIRArgs; }

		std::pair<unsigned, unsigned> getIRArgs(unsigned ArgNo) const
		{
			assert(ArgNo < ArgInfo.size() && "Index out of range.");
			return std::make_pair(ArgInfo[ArgNo].FirstArgIndex, ArgInfo[ArgNo].NumberOfArgs);
		}
	private:
		void construct(const CGFunctionInfo& FI);
	};
	void ASTToIRMapping::construct(const CGFunctionInfo& FI)
	{
		auto RetAI = FI.getReturnInfo();
		unsigned IRCursorArgNo = 0;
		if (RetAI->getKind() == ArgABIInfo::InDirect)
		{
			// SRet
			HasSRet = true;
			IRCursorArgNo++;
		}
		
		ArgInfo.push_back(IRArgs(0, 1));

		unsigned ArgNo = 0;
		for (auto item : FI.getArgsInfo())
		{
			switch (item->getKind())
			{
			case ArgABIInfo::Direct:
				// coerce or flatten.
				if (item->getType()->getKind() == TypeKind::USERDEFIED || 
					item->getType()->getKind() == TypeKind::ANONYMOUS)
				{
					auto StructTy = std::dynamic_pointer_cast<StructType>(item->getType());
					assert(StructTy && "Type error when ASTToIRMapping!");
					ArgInfo.push_back(IRArgs(IRCursorArgNo, StructTy->getNumElements()));
					IRCursorArgNo += StructTy->getNumElements();
				}
				else
				{
					ArgInfo.push_back(IRArgs(IRCursorArgNo++, 1));
				}
				break;
			case ArgABIInfo::InDirect:
				ArgInfo.push_back(IRArgs(IRCursorArgNo++, 1));
				break;
			case ArgABIInfo::Ignore:
				ArgInfo.push_back(IRArgs(IRCursorArgNo, 0));
				break;
			default:
				assert(0 && "Unrechable code!");
				break;
			}
		}
		TotalIRArgs = IRCursorArgNo;
	}
}

/// \brief getFunctionType - Create FunctionType for CGFunctionInfo.
/// e.g.	class Node { var x:int; var y:int; };
///			func foo(parm:Node, length:int, Agg:{int, {bool, int}, int}) -> Node 
///			{
///				var n:Node;
///				n.x = 10;
///				n.y = 4;
///				return n;
///			}
///
///			moses IR as below:
///			
///			%struct.Node = type {int, int}
///			%anony.1 = type {int, {bool, int}, int}
///			define void @foo(%struct.Node* sret retvalue, int %struct.Node.x, int %struct.Node.y,
///							%anony.1* byval parm)
///			{
///				%1 = alloca int
///				%2 = alloca int
///				store int %struct.Node.x, int* %1;
///				store int %struct.Node.y, int* %2;
///				; GEP
///				ret
///			}
FuncTypePtr CodeGenTypes::getFunctionType(std::shared_ptr<CGFunctionInfo const> Info)
{
	// (1) Create the llvm::FunctionType
	TyPtr ResultTy = nullptr;
	auto RetInfo = Info->getReturnInfo();
	auto RetTy = RetInfo->getType();
	switch (RetInfo->getKind())
	{
	case ArgABIInfo::Kind::Ignore:
		ResultTy = IR::Type::getVoidType();
		break;
	case ArgABIInfo::Kind::InDirect:
		ResultTy = PointerType::get(ConvertType(RetTy));
		break;
	case ArgABIInfo::Kind::Direct:
		ResultTy = ConvertType(RetTy);
		break;
	default:
		break;
	}

	ASTToIRMapping IRFunctionArgs(*Info);
	std::vector<TyPtr> ArgTypes;
	// Add return type.
	ArgTypes.push_back(ResultTy);

	// Add parm type.
	auto ArgsInfo = Info->getArgsInfo();
	auto NumArgs = ArgsInfo.size();
	for (unsigned i = 1; i < NumArgs; i++)
	{
		unsigned FirstIRArg, NumIRArgs;
		std::tie(FirstIRArg, NumIRArgs) = IRFunctionArgs.getIRArgs(i);
		switch (ArgsInfo[i]->getKind())
		{
		case ArgABIInfo::Kind::Direct:
			if (ArgsInfo[i]->getType()->getKind() == TypeKind::USERDEFIED ||
				ArgsInfo[i]->getType()->getKind() == TypeKind::ANONYMOUS)
			{
				// Can be flattened.
				if (ArgsInfo[i]->canBeFlattened())
				{
					// This is the only case: {int/bool, int/bool}
					// To Do: for anonymous type, may have this situation 
					//		  {{int/bool}, {int/bool}}
					// Or
					//		  for class type, {{{{type}}}}
					for ()
					{

					}
					ArgTypes.push_back();
				}
				else
				{

				}
				// Or 
				// coerce.
			}
			break;
		case ArgABIInfo::Kind::InDirect:
			break;
		case ArgABIInfo::Ignore:
			break;
		default:
			break; 
		}
		if (NumIRArgs > 1)
		{
			// must be flattened;
		}
	}
}

/// \brief Generate CGFunctionInfo for FunctionDecl.
/// Note: Moses have no function declaration, so there is no indirect recursion.
std::shared_ptr<CGFunctionInfo const> CodeGenTypes::arrangeFunctionInfo(const FunctionDecl *FD)
{
	// (1) Check whether the CGFunctionInfo exists.
	auto iter = FunctionInfos.find(FD);
	if (iter != FunctionInfos.end())
		return iter->second;

	// (2) otherwise, save this info.
	auto CGFuncInfo = CGFunctionInfo::create(FD);
	FunctionInfos.insert({ FD, CGFuncInfo });
	return CGFuncInfo;
}