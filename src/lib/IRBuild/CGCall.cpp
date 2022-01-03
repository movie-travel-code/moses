//===------------------------------CGCall.cpp-----------------------------===//
//
// This file wrap the information about a call or function definition.
//
//===---------------------------------------------------------------------===//
#include "IRBuild/CodeGenTypes.h"
#include "IRBuild/IRBuilder.h"

using namespace IR;
using namespace IRBuild;
extern void print(std::shared_ptr<IR::Value> V);
namespace {
/// Encapsulates information about the way function from CGFunctionInfo should
/// be passed to actual IR function.
/// e.g.	[parm1, parm2, ..., parmn] -> ret
///                ||
///                \/
///			[sret ,type1, coerce-to-type2, ..., {flatten.1,
/// flatten.2}] 	ASTToIRMapping is the key to generate function.
class ASTToIRMapping {
  unsigned TotalIRArgs;
  bool HasSRet;
  /// Arguments of IR function correspoding to single AST argument.
  struct IRArgs {
    // Argument is expanded to IR arguments at positions.
    // [FirstArgIndex, FirstArgIndex + NumberOfArgs).
    unsigned FirstArgIndex;
    unsigned NumberOfArgs;

    IRArgs(unsigned first, unsigned length)
        : FirstArgIndex(first), NumberOfArgs(length) {}
  };
  std::vector<IRArgs> ArgInfo;

public:
  ASTToIRMapping(const CGFunctionInfo &FI) : TotalIRArgs(0), HasSRet(false) {
    construct(FI);
  }
  bool hasSRetArg() const { return HasSRet; }
  unsigned totalIRArgs() const { return TotalIRArgs; }

  std::pair<unsigned, unsigned> getIRArgs(unsigned ArgNo) const {
    assert(ArgNo < ArgInfo.size() && "Index out of range.");
    return std::make_pair(ArgInfo[ArgNo].FirstArgIndex,
                          ArgInfo[ArgNo].NumberOfArgs);
  }

private:
  void construct(const CGFunctionInfo &FI);
};
void ASTToIRMapping::construct(const CGFunctionInfo &FI) {
  auto RetAI = FI.getReturnInfo();
  unsigned IRCursorArgNo = 0;
  if (RetAI->getKind() == ArgABIInfo::Kind::InDirect) {
    // SRet
    HasSRet = true;
    IRCursorArgNo++;
    ArgInfo.push_back(IRArgs(0, 1));
  }

  for (auto item : FI.getArgsInfo()) {
    switch (item->getKind()) {
    case ArgABIInfo::Kind::Direct:
      // coerce or flatten.
      if (item->getType()->getKind() == TypeKind::USERDEFIED ||
          item->getType()->getKind() == TypeKind::ANONYMOUS) {
        auto ty = item->getType();
        ArgInfo.push_back(IRArgs(IRCursorArgNo, ty->MemberNum()));
        IRCursorArgNo += ty->MemberNum();
      } else {
        ArgInfo.push_back(IRArgs(IRCursorArgNo++, 1));
      }
      break;
    case ArgABIInfo::Kind::InDirect:
      ArgInfo.push_back(IRArgs(IRCursorArgNo++, 1));
      break;
    case ArgABIInfo::Kind::Ignore:
      ArgInfo.push_back(IRArgs(IRCursorArgNo, 0));
      break;
    default:
      assert(0 && "Unrechable code!");
      break;
    }
  }
  TotalIRArgs = IRCursorArgNo;
}
} // namespace

/// EmitFunctionPrologue - This function mainly generate prologue code.
///	e.g.	define i32 func(i32 lhs, i32 rhs)
///      {
///        %1 = alloca i32   ----------> This is for lhs.
///        %2 = alloca i32   ----------> This is for rhs.
///      }
///
/// Note: moses's parameter passing will become very complex. We allow the
/// parameter of anonymous type. And we will use reference modle later.
///	e.g.	%struct.1 = {i32, {i1, i32}}
/// 		define i32 func(%struct.1 lhs) {}
///							~~~~~~~~
/// Therefore, we should refer to 'inalloca' attribute in future.
void ModuleBuilder::EmitFunctionPrologue(CGFuncInfoConstPtr FunInfo,
                                         std::shared_ptr<Function> fun) {
  // Create a pointer value for every parameter declaration. This usually
  // entails copying one or more IR arguments into an alloca.
  // e.g.		define i32 @func(i32 %lhs, i32 %rhs)
  //			{
  //				entry:
  //					%retval = alloca i32
  //					%lhs.addr = alloca i32
  //					%rhs.addr = alloca i32
  //					~~~~~~~~~~~~~~~~~~~~~~
  //					store i32 %lhs, i32* %lhs.addr
  //					store i32 %rhs, i32* %rhs.addr
  //					~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  //			}
  auto FuncType = fun->getFunctionType();
  bool isSRet = false;
  if (FunInfo->getReturnInfo()->getKind() == ArgABIInfo::Kind::InDirect) {
    fun->getArg(0)->setName(getCurLocalName("agg.result"));
    isSRet = true;
  }

  ASTToIRMapping IRFunctionArgs(*FunInfo);
  for (unsigned i = 0; i < FunInfo->getArgNums(); i++) {
    auto ArgInfo = FunInfo->getArgABIInfo(i);
    auto Arg = CurFunc->CurFuncDecl->getParmDecl(i).get();
    unsigned FirstIRArg, NumIRArgs;
    std::tie(FirstIRArg, NumIRArgs) =
        IRFunctionArgs.getIRArgs(isSRet ? i + 1 : i);
    switch (ArgInfo->getKind()) {
    case ArgABIInfo::Kind::InDirect: {
      std::shared_ptr<Value> V = fun->getArg(FirstIRArg);
      EmitParmDecl(Arg, V);
      break;
    }
    case ArgABIInfo::Kind::Direct: {
      std::shared_ptr<Value> V = fun->getArg(FirstIRArg);
      auto ArgType = Types.ConvertType(ArgInfo->getType());
      if (ArgType->isAggregateType()) {
        // If this structure was wxpanded into multiple arguments then we
        // need to create a temporary and reconstruct it from the arguments.
        std::string Name = Arg->getName();
        std::shared_ptr<Value> Temp =
            CreateAlloca(ArgType, LocalInstNamePrefix + Name + ".addr");

        std::vector<std::shared_ptr<Value>> SubArgs;
        for (unsigned index = FirstIRArg; index < FirstIRArg + NumIRArgs;
             index++) {
          SubArgs.push_back(fun->getArg(index));
          fun->getArg(index)->setName(Name + "." +
                                      std::to_string(index - FirstIRArg));
        }
        ExpandTypeFromArgs(ArgInfo->getType(), LValue::MakeAddr(Temp), SubArgs);
        EmitParmDecl(Arg, Temp);
      } else {
        EmitParmDecl(Arg, fun->getArg(FirstIRArg));
      }
      break;
    }
    case ArgABIInfo::Kind::Ignore:
      EmitParmDecl(Arg, CreateAlloca(Types.ConvertType(ArgInfo->getType())));
      break;
    }
  }
}

/// EmitFunctionEpilog - Emit the code to return the given temporary.
void ModuleBuilder::EmitFunctionEpilogue(CGFuncInfoConstPtr CGFnInfo) {
  std::shared_ptr<Value> RV = nullptr;
  auto RetInfo = CGFnInfo->getReturnInfo();

  switch (RetInfo->getKind()) {
  case ArgABIInfo::Kind::Direct:
    RV = CreateLoad(CurFunc->ReturnValue);
    break;
  case ArgABIInfo::Kind::InDirect:
    EmitAggregateCopy(CurFunc->CurFn->getArg(0), CurFunc->ReturnValue,
                      RetInfo->getType());
    break;
  case ArgABIInfo::Kind::Ignore:
    break;
  default:
    break;
  }

  if (RV) {
    auto ret = CreateRet(RV);
  } else {
    auto ret = CreateRetVoid();
  }
}

/// ExpandTypeFromArgs - Reconstruct a structure of type \arg Ty
/// from function arguments into \arg Dst.
void ModuleBuilder::ExpandTypeFromArgs(
    std::shared_ptr<ASTType> ASTTy, LValue LV,
    std::vector<std::shared_ptr<Value>> &SubArgs) {
  assert(ASTTy->getKind() == TypeKind::ANONYMOUS ||
         ASTTy->getKind() == TypeKind::USERDEFIED &&
             "Can only expand class types.");
  auto Addr = LV.getAddress();
  for (unsigned i = 0, size = ASTTy->MemberNum(); i < size; i++) {
    auto ty = Types.ConvertType((*ASTTy)[i].first);
    auto LV = LValue::MakeAddr(CreateGEP(ty, Addr, i));
    EmitStoreThroughLValue(RValue::get(SubArgs[i]), LV);
  }
}

/// ExpandTypeToArgs - Expand an RValue \arg Src, with the IR type for
/// \arg Ty, into individual arguments on the provided vector \arg Args.
void ModuleBuilder::ExpandTypeToArgs(
    std::shared_ptr<ASTType> ASTTy, RValue Src,
    std::vector<std::shared_ptr<Value>> &Args) {
  assert(ASTTy->getKind() == TypeKind::ANONYMOUS ||
         ASTTy->getKind() == TypeKind::USERDEFIED &&
             "Can only expand class types.");
  auto Addr = Src.getAggregateAddr();
  for (unsigned i = 0, size = ASTTy->MemberNum(); i < size; i++) {
    auto ty = Types.ConvertType((*ASTTy)[i].first);
    auto forPrint = CreateGEP(ty, Addr, i);

    auto LV = LValue::MakeAddr(forPrint);

    auto RV = EmitLoadOfLValue(LV);
    Args.push_back(RV.getScalarVal());
  }
}

/// CreateCoercedStore - Create a store to \arg DstPtr from \arg Src,
/// where the souece and destination may have different types.
/// e.g.	class A {var mem:int;};		--coerce to--> int
///			func ret() -> A { }			--coerce to--> func ret()
///-> int {}
/// int-value ----> class-A-value
void ModuleBuilder::CreateCoercedStore(std::shared_ptr<Value> Src,
                                       std::shared_ptr<Value> DestPtr) {
  // Handle specially and foolish
  // (1) Create GEP instruction for DestPtr
  //     DestPtr , 0, 0
  // (2) EmitStoreOfScalar()
  auto gep = CreateGEP(Src->getType(), DestPtr, 0);
  auto store = CreateStore(Src, gep);
}

/// \brief EmitCall - Emit code for CallExpr.
RValue ModuleBuilder::EmitCall(const FunctionDecl *FD,
                               std::shared_ptr<Value> FuncAddr,
                               const std::vector<ExprASTPtr> &ArgExprs) {
  CallArgList Args;
  // (1) EmitCallArgs().
  EmitCallArgs(Args, ArgExprs);

  // (2) EmitCall() -> EmitCallArgs.
  auto CGFunInfo = Types.arrangeFunctionInfo(FD);
  return EmitCall(FD, CGFunInfo, FuncAddr, Args);
}

/// \brief EmitCall - Generate a call of the given funciton.
/// e.g.	func add(n : Node) -> int {}    ----flattend---->   define @add(int
/// n.1, int n.2) {}
///
///	When we come across 'add(n)', we should emit 'n' first and emit
/// callexpr.
RValue ModuleBuilder::EmitCall(const FunctionDecl *FD,
                               CGFuncInfoConstPtr CGFunInfo,
                               std::shared_ptr<Value> FuncAddr,
                               CallArgList CallArgs) {
  std::vector<std::shared_ptr<Value>> Args;
  // Handle struct-return functions by passing a pointer to the location that we
  // would like to return into.
  auto RetInfo = CGFunInfo->getReturnInfo();

  // If the call return a temporary with struct return, create a temporary
  // alloca to hold the result.
  if (RetInfo->getKind() == ArgABIInfo::Kind::InDirect) {
    auto forPrint = CreateAlloca(Types.ConvertType(RetInfo->getType()),
                                 getCurLocalName("ret.tmp"));
    Args.push_back(forPrint);
  }

  auto ArgsInfo = CGFunInfo->getArgsInfo();
  assert(ArgsInfo.size() == CallArgs.size() && "Arguments number don't match!");
  unsigned ArgsNum = ArgsInfo.size();
  for (unsigned i = 0; i < ArgsNum; i++) {
    auto ArgInfo = ArgsInfo[i];
    auto ArgVal = CallArgs[i].first;
    auto ty = Types.ConvertType(ArgInfo->getType());
    switch (ArgInfo->getKind()) {
    case ArgABIInfo::Kind::Direct:
      if (ty->isAggregateType()) {
        ExpandTypeToArgs(ArgInfo->getType(), ArgVal, Args);
      } else {
        Args.push_back(CallArgs[i].first.getScalarVal());
      }
      break;
    case ArgABIInfo::Kind::Ignore:
      break;
    case ArgABIInfo::Kind::InDirect:
      Args.push_back(CallArgs[i].first.getAggregateAddr());
      break;
    default:
      break;
    }
  }

  // Is this call a builtin function call.
  if (FD->isBuiltin()) {
    // FIXME: In the future, we should use a more natural approach to handle
    // builtin call generation.
    if (FD->getFDName() == "print") {
      auto IRPrint = Context.getPrint();
      auto Call = CreateIntrinsic(IRPrint, Args);
      return RValue::get(0);
    }
  }

  auto CallRest = CreateCall(FuncAddr, Args);
  switch (RetInfo->getKind()) {
  case ArgABIInfo::Kind::InDirect:
    return RValue::getAggregate(Args[0]);
  case ArgABIInfo::Kind::Direct:
    // (1) AggregateType coerce
    // (2) BuiltinType
    if (Types.ConvertType(RetInfo->getType())->isAggregateType()) {
      std::shared_ptr<Value> V =
          CreateAlloca(Types.ConvertType(RetInfo->getType()), "%coerce");
      CreateCoercedStore(CallRest, V);
      return RValue::getAggregate(V);
    } else {
      return RValue::get(CallRest);
    }
  case ArgABIInfo::Kind::Ignore:
    break;
  }
  return RValue::get(0);
}

// EmitCallArgs - Emit call arguments for a function.
void ModuleBuilder::EmitCallArgs(CallArgList &CallArgs,
                                 const std::vector<ExprASTPtr> &ArgExprs) {
  std::shared_ptr<Value> V = nullptr;
  for (auto item : ArgExprs) {
    auto ty = Types.ConvertType(item->getType());
    if (ty->isAggregateType()) {
      auto AggTemp = CreateAlloca(ty, getCurLocalName("agg.tmp"));
      EmitAggExpr(item.get(), AggTemp);
      V = AggTemp;
    } else {
      V = EmitScalarExpr(item.get());
    }
    CallArgs.push_back({RValue::get(V), item->getType()});
  }
}

//===--------------------------------------------------------------------------===//
// Implements the ArgABIInfo
std::shared_ptr<ArgABIInfo> ArgABIInfo::Create(std::shared_ptr<ASTType> type,
                                               Kind kind) {
  return std::make_shared<ArgABIInfo>(type, kind);
}

//===--------------------------------------------------------------------------===//
// Implements the CGFunctionInfo below.
CGFunctionInfo::CGFunctionInfo(
    MosesIRContext &Ctx,
    std::vector<std::pair<std::shared_ptr<ASTType>, std::string>> ArgsTy,
    std::shared_ptr<ASTType> RetTy) {
  // Generate the ArgABIInfo.
  // (1) Create the ArgABIInfo for Return Type.
  ReturnInfo = classifyReturnTye(Ctx, RetTy);

  // (2) Create the ArgABIInfos for Args.
  for (auto item : ArgsTy) {
    ArgInfos.push_back(classifyArgumentType(Ctx, item.first, item.second));
  }
}

/// \brief classifyReturnType - compute the ArgABIInfo for ReturnType.
///	Rule:	int/bool                      ----> Direct
///       void                          ----> Ignore
///       struct{int/bool}              ----> Direct(coerce to int)
///       struct{int/bool, int/bool}    ----> InDirect(sret hidden pointer)
AAIPtr CGFunctionInfo::classifyReturnTye(MosesIRContext &Ctx,
                                         std::shared_ptr<ASTType> RetTy) {
  if (RetTy->getKind() == TypeKind::VOID)
    return std::make_shared<ArgABIInfo>(RetTy, ArgABIInfo::Kind::Ignore);

  if (RetTy->getKind() == TypeKind::INT)
    return std::make_shared<ArgABIInfo>(RetTy, ArgABIInfo::Kind::Direct, "",
                                        IR::Type::getIntType(Ctx));
  if (RetTy->getKind() == TypeKind::BOOL)
    return std::make_shared<ArgABIInfo>(RetTy, ArgABIInfo::Kind::Direct, "",
                                        IR::Type::getIntType(Ctx));

  // small structures which are register sized are generally returned
  // in register.
  if (RetTy->size() <= 32) {
    auto coreTy = RetTy->StripOffShell();
    if (coreTy->getKind() == TypeKind::INT)
      return std::make_shared<ArgABIInfo>(RetTy, ArgABIInfo::Kind::Direct, "",
                                          IR::Type::getIntType(Ctx));
    if (coreTy->getKind() == TypeKind::BOOL)
      return std::make_shared<ArgABIInfo>(RetTy, ArgABIInfo::Kind::Direct, "",
                                          IR::Type::getIntType(Ctx));
    if (coreTy->getKind() == TypeKind::VOID)
      return std::make_shared<ArgABIInfo>(RetTy, ArgABIInfo::Kind::Direct, "",
                                          IR::Type::getVoidType(Ctx));
  }
  return std::make_shared<ArgABIInfo>(RetTy, ArgABIInfo::Kind::InDirect,
                                      "%ret.addr");
}

/// \brief classifyArgumentType - compute the ArgABIInfo for ArgumentType.
/// Rule:	int/bool						---->
/// Direct
///			void ----> Ignore 			struct{int/bool}
/// ----> Direct(coerce to
/// int-i32) 			struct{int/bool, int/bool}		----> Direct(flatten
/// int, int) 			struct{int/bool, int/bool,,,}	----> Indirect(hidden pointer)
AAIPtr CGFunctionInfo::classifyArgumentType(MosesIRContext &Ctx,
                                            std::shared_ptr<ASTType> ArgTy,
                                            const std::string &Name) {
  if (ArgTy->getKind() == TypeKind::VOID)
    return std::make_shared<ArgABIInfo>(ArgTy, ArgABIInfo::Kind::Ignore, Name);

  if (ArgTy->getKind() == TypeKind::BOOL)
    return std::make_shared<ArgABIInfo>(ArgTy, ArgABIInfo::Kind::Direct, Name,
                                        IR::Type::getBoolType(Ctx));
  if (ArgTy->getKind() == TypeKind::INT)
    return std::make_shared<ArgABIInfo>(ArgTy, ArgABIInfo::Kind::Direct, Name,
                                        IR::Type::getIntType(Ctx));

  // small structures which are register sized are generally returned
  // in register or flatten as two registers.
  if (ArgTy->size() <= 32) {
    // case 1: class A { var m:bool; };
    // case 2: class A { var m:bool; }; class B{ var m:A; };
    // case 3: var num = {{{int}}}
    // To Do: We need to strip off the '{' and '}' to get the core type.
    return std::make_shared<ArgABIInfo>(ArgTy, ArgABIInfo::Kind::Direct, Name,
                                        IR::Type::getIntType(Ctx));
  }

  if (ArgTy->size() <= 64)
    return std::make_shared<ArgABIInfo>(ArgTy, ArgABIInfo::Kind::Direct, Name,
                                        nullptr, true);
  return std::make_shared<ArgABIInfo>(ArgTy, ArgABIInfo::Kind::InDirect,
                                      ModuleBuilder::LocalInstNamePrefix +
                                          Name + ".addr");
}

CGFuncInfoConstPtr CGFunctionInfo::create(MosesIRContext &Ctx,
                                          const FunctionDecl *FD) {
  std::vector<std::pair<std::shared_ptr<ASTType>, std::string>> ArgsTy;
  for (auto item : FD->getParms()) {
    ArgsTy.push_back({item->getDeclType(), item->getName()});
  }
  return std::make_shared<CGFunctionInfo>(Ctx, ArgsTy, FD->getReturnType());
}

const std::shared_ptr<ASTType> CGFunctionInfo::getParm(unsigned index) const {
  assert(index <= getArgNums() - 1 &&
         "Index out of range when we get FunctionInfo.");
  return ArgInfos[index]->getType();
}

ArgABIInfo::Kind CGFunctionInfo::getKind(unsigned index) const {
  assert(index <= getArgNums() - 1 &&
         "Index out of range when we get FunctionInfo.");
  return ArgInfos[index]->getKind();
}

const AAIPtr CGFunctionInfo::getArgABIInfo(unsigned index) const {
  assert(index <= getArgNums() - 1 &&
         "Index out of range when we get FunctionInfo.");
  return ArgInfos[index];
}

/// \brief getArgNames - return the argument's name(include return value).
/// To Do: efficiency
std::vector<std::string> CGFunctionInfo::getArgNames() const {
  std::vector<std::string> Names;
  Names.push_back(ReturnInfo->getArgName());
  for_each(ArgInfos.begin(), ArgInfos.end(), [&Names](const AAIPtr &arginfo) {
    Names.push_back(arginfo->getArgName());
  });
  return Names;
}

/// \brief getFunctionType - Create FunctionType for CGFunctionInfo.
/// e.g.	class Node { var x:int; var y:int; };
///			func foo(parm:Node, length:int, Agg:{int, {bool, int}, int})
///-> Node
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
///			define void @foo(%struct.Node* sret retvalue, int
///%struct.Node.x, int %struct.Node.y, %anony.1* byval parm)
///			{
///				%1 = alloca int
///				%2 = alloca int
///				store int %struct.Node.x, int* %1;
///				store int %struct.Node.y, int* %2;
///				; GEP
///				ret
///			}
GetFuncTypeRet CodeGenTypes::getFunctionType(const FunctionDecl *FD,
                                             CGFuncInfoConstPtr Info) {
  std::vector<std::string> ArgNames;
  std::vector<TyPtr> ArgTypes;

  // (1) Create the llvm::FunctionType
  TyPtr ResultTy = nullptr;
  auto RetInfo = Info->getReturnInfo();
  auto RetTy = RetInfo->getType();
  switch (RetInfo->getKind()) {
  case ArgABIInfo::Kind::Ignore:
    ResultTy = IR::Type::getVoidType(IRCtx);
    break;
  case ArgABIInfo::Kind::InDirect:
    ArgTypes.push_back(PointerType::get(ConvertType(RetTy)));
    ArgNames.push_back(RetInfo->getArgName());
    ResultTy = IR::Type::getVoidType(IRCtx);
    break;
  case ArgABIInfo::Kind::Direct:
    ResultTy = ConvertType(RetTy);
    break;
  default:
    break;
  }

  ASTToIRMapping IRFunctionArgs(*Info);

  // Add parm type.
  auto ArgsInfo = Info->getArgsInfo();

  for (unsigned i = 0, NumArgs = ArgsInfo.size(); i < NumArgs; i++) {
    unsigned FirstIRArg, NumIRArgs;
    std::tie(FirstIRArg, NumIRArgs) = IRFunctionArgs.getIRArgs(i);
    switch (ArgsInfo[i]->getKind()) {
    case ArgABIInfo::Kind::Direct:
      if (ArgsInfo[i]->getType()->getKind() == TypeKind::USERDEFIED ||
          ArgsInfo[i]->getType()->getKind() == TypeKind::ANONYMOUS) {
        // Can be flattened.
        // Shit Code! Shit Code! Shit Code!
        unsigned count = 0;
        if (ArgsInfo[i]->canBeFlattened()) {
          auto StructTy = std::dynamic_pointer_cast<UserDefinedType>(
              ArgsInfo[i]->getType());
          if (StructTy) {
            for (auto item : StructTy->getMemberTypes()) {
              auto StripOffTy = item.first->StripOffShell();
              ArgTypes.push_back(
                  ConvertType(StripOffTy == nullptr ? item.first : StripOffTy));
              ArgNames.push_back(ArgsInfo[i]->getArgName() + "." +
                                 std::to_string(count++));
            }
            break;
          }
          auto AnonyTy =
              std::dynamic_pointer_cast<AnonymousType>(ArgsInfo[i]->getType());
          count = 0;
          if (AnonyTy) {
            for (auto item : AnonyTy->getSubTypes()) {
              ArgTypes.push_back(ConvertType(item->StripOffShell()));
              ArgNames.push_back(ArgsInfo[i]->getArgName() + "." +
                                 std::to_string(count++));
            }
            break;
          }
          assert(0 && "Unreachable code!");
        } else {
          ArgTypes.push_back(ArgsInfo[i]->getCoerceeToType());
          ArgNames.push_back(ArgsInfo[i]->getArgName());
        }
      } else {
        ArgTypes.push_back(ConvertType(ArgsInfo[i]->getType()));
        ArgNames.push_back(ArgsInfo[i]->getArgName());
      }
      break;
    case ArgABIInfo::Kind::InDirect:
      ArgTypes.push_back(PointerType::get(ConvertType(ArgsInfo[i]->getType())));
      ArgNames.push_back(ArgsInfo[i]->getArgName() + ".addr");
      break;
    case ArgABIInfo::Kind::Ignore:
      ArgTypes.push_back(IR::Type::getVoidType(IRCtx));
      ArgNames.push_back(ArgsInfo[i]->getArgName());
      break;
    }
  }
  // To Do: Save the FunctionType.
  auto FuncTy = FunctionType::get(ResultTy, ArgTypes);
  FunctionTypes.insert({FD, FuncTy});
  return std::make_pair(FuncTy, ArgNames);
}

/// \brief Generate CGFunctionInfo for FunctionDecl.
/// Note: Moses have no function declaration, so there is no indirect recursion.
CGFuncInfoConstPtr CodeGenTypes::arrangeFunctionInfo(const FunctionDecl *FD) {
  // (1) Check whether the CGFunctionInfo exists.
  auto iter = FunctionInfos.find(FD);
  if (iter != FunctionInfos.end())
    return iter->second;

  // (2) otherwise, save this info.
  auto CGFuncInfo = CGFunctionInfo::create(IRCtx, FD);
  FunctionInfos.insert({FD, CGFuncInfo});
  return CGFuncInfo;
}