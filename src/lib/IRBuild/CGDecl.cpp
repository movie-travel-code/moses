//===--------------------------------CGDecl.cpp---------------------------===//
//
// This contains code to emit Decl nodes.
//
//===---------------------------------------------------------------------===//
#include "IRBuild/IRBuilder.h"
using namespace ast;
using namespace IR;
using namespace IRBuild;
using namespace CodeGen;
extern void print(std::shared_ptr<IR::Value> V);
/// EmitVarDecl - This method handles emission of variable declaration inside
/// a function.  Emit code and set the symbol table entry about this var.
///		func add() -> int
///		{
///			var num : int;
///		}
void ModuleBuilder::EmitLocalVarDecl(const VarDecl *var) {
  // (1) Create AllocInst.
  std::shared_ptr<Value> DeclPtr = EmitLocalVarAlloca(var);

  // (2) If 'var' have Initialization expression, emit it.
  // Note: We only handle the built-in type.
  if (auto init = var->getInitExpr()) {
    auto ty = Types.ConvertType(init->getType());
    if (ty->isAggregateType()) {
      EmitAggExpr(init.get(), DeclPtr);
    } else {
      std::shared_ptr<Value> V = EmitScalarExpr(init.get());
      EmitStoreOfScalar(V, DeclPtr);
    }
  }
}

/// \brief EmitLocalVarAlloca - Emit tha alloca for a local variable.
std::shared_ptr<AllocaInst>  ModuleBuilder::EmitLocalVarAlloca(const VarDecl *var) {
  std::shared_ptr<ASTType> Ty = var->getDeclType();

  IRTyPtr IRTy = Types.ConvertType(Ty);
  std::shared_ptr<AllocaInst>  allocInst =
      CreateAlloca(IRTy, LocalInstNamePrefix + var->getName() + ".addr");

  if (std::shared_ptr<VariableSymbol> varSym =
          std::dynamic_pointer_cast<VariableSymbol>(
              CurScope->Resolve(var->getName()))) {
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
void ModuleBuilder::EmitParmDecl(const VarDecl *VD, std::shared_ptr<Value> Arg) {
  std::shared_ptr<Value> DeclPtr;
  // A fixed sized single-value variable becomes an alloca in the entry block.
  IRTyPtr Ty = Types.ConvertType(VD->getDeclType());

  std::string Name = VD->getName();
  if (Ty->isSingleValueType()) {
    // e.g. define func(i32 %lhs, i32 %rhs)
    //		{
    //			%lhs.addr = alloca i32
    //		}
    Name = LocalInstNamePrefix + Name + ".addr";
    DeclPtr = CreateAlloca(Ty);

    // Store the intial value into the alloca.
    EmitStoreOfScalar(Arg, DeclPtr);
  } else {
    // otherwise, if this is an aggregate, just use the input pointer.
    DeclPtr = Arg;
  }
  Arg->setName(Name);

  // (1) Get the ParmDecl's SymbolTable Entry.
  auto SymEntry = CurScope->Resolve(VD->getName());
  assert(SymEntry != nullptr && "Parameter declaration doesn't exists.");
  if (std::shared_ptr<ParmDeclSymbol> sym =
          std::dynamic_pointer_cast<ParmDeclSymbol>(SymEntry)) {
    assert((sym->getAllocaInst() == nullptr) &&
           "Symbol's alloca instruciton already exists.");
    sym->setAllocaInst(DeclPtr);
  }
}

void ModuleBuilder::EmitScalarInit([[maybe_unused]] const Expr *init,
                                   [[maybe_unused]] const VarDecl *D,
                                   [[maybe_unused]] LValue lvalue) {}

//===---------------------------------------------------------------------===//
// Generate code for function declaration.
//===---------------------------------------------------------------------===//
void ModuleBuilder::EmitFunctionDecl(const FunctionDecl *FD) {
  // generate function info.
  std::shared_ptr<CGFunctionInfo const> FI = Types.arrangeFunctionInfo(FD);

  auto TypeAndName = Types.getFunctionType(FD, FI);

  CurFunc->CGFnInfo = FI;
  CurFunc->CurFuncDecl = const_cast<FunctionDecl *>(FD);
  CurFunc->FnRetTy = Types.ConvertType(FD->getReturnType());
  std::shared_ptr<Function> func =
      Function::create(TypeAndName.first, FD->getFDName(), TypeAndName.second);
  CurFunc->CurFn = func;
  IRs.push_back(func);
  // (1) Start function.
  // Note: we need switch the scope.
  //    e.g.    var num = 10;                           ----> Old Scope.
  //            func add(lhs:int, rhs : int) -> int     ----> New Scope.
  //            {
  //              ...
  //            }
  auto Sym = CurScope->Resolve(FD->getFDName());
  assert(Sym != nullptr && "Funciton symbol can't be null.");
  std::shared_ptr<FunctionSymbol> FuncSym =
      std::dynamic_pointer_cast<FunctionSymbol>(Sym);
  assert(FuncSym != nullptr && "Funciton symbol can't be null.");
  FuncSym->setFuncAddr(func);
  CurScope = FuncSym->getScope();

  StartFunction(FI, func);

  // (2) Emit the function body.
  EmitFunctionBody(FD->getCompoundBody());

  // (3) Finish function.
  FinishFunction();
  CurFunc->CurFn = nullptr;

  // (4) Switch the scope back.
  CurScope = CurScope->getParent();
}

/// \brief Handle the start of function.
void ModuleBuilder::StartFunction(std::shared_ptr<CGFunctionInfo const> FnInfo,
                                  std::shared_ptr<Function> Fn) {
  EntryBlock = CreateBasicBlock("entry", Fn);
  // EmitBlock(EntryBB);
  Fn->addBB(EntryBlock);
  SetInsertPoint(EntryBlock);
  AllocaInsertPoint = InsertPoint;
  isAllocaInsertPointSetByNormalInsert = false;
  TempCounter = 0;

  CurFunc->ReturnBlock = CreateBasicBlock("return", CurFunc->CurFn);
  auto RetTy = FnInfo->getReturnInfo()->getType();
  if (RetTy->getKind() == TypeKind::VOID) {
    // Void type; nothing to return
    CurFunc->ReturnValue = nullptr;
  } else {
    CurFunc->ReturnValue = CreateAlloca(Types.ConvertType(RetTy), "%retval");
  }
  EmitFunctionPrologue(FnInfo, Fn);
}

void ModuleBuilder::FinishFunction() {
  EmitReturnBlock();
  EmitFunctionEpilogue(CurFunc->CGFnInfo);
}

std::shared_ptr<Value> ModuleBuilder::visit(const VarDecl *VD) {
  EmitLocalVarDecl(VD);
  return nullptr;
}

/// \brief Handle function declaration.
std::shared_ptr<Value> ModuleBuilder::visit(const FunctionDecl *FD) {
  // (1) switch the scope and save the context-info.
  // e.g.       var num = 10;                           ----> Old Scope.
  //            func add(lhs:int, rhs:int) -> int       ----> New Scope.
  //            {
  //              ...
  //            }
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

std::shared_ptr<Value> ModuleBuilder::visit([[maybe_unused]] const UnpackDecl *UD) {
  return nullptr;
}