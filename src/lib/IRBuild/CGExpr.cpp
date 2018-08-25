//===--------------------------CGExpr.cpp---------------------------------===//
//
// This contains code to emit Expr nodes.
//
//===---------------------------------------------------------------------===//
#include "include/IRBuild/IRBuilder.h"
using namespace compiler::IR;
using namespace compiler::IRBuild;
extern void print(std::shared_ptr<compiler::IR::Value> V);
/// \brief Emit branch condition.
void ModuleBuilder::EmitBranchOnBoolExpr(ExprASTPtr Cond, BBPtr TrueBlock,
                                         BBPtr FalseBlock) {
  if (BinaryPtr CondBOp = std::dynamic_pointer_cast<BinaryExpr>(Cond)) {
    // Handle X && Y in a conditon.
    if (CondBOp->getOpcode() == "&&") {
      // If we have "true && X", simplify the code. "0 && X" would have constant
      // folded if the case was simple enough.
      bool result;
      // Here the results of the evaluation can only be ture value.
      if (evaluator.EvaluateAsBooleanCondition(CondBOp->getLHS(), result))
        // br(true && X) -> br(X).
        return EmitBranchOnBoolExpr(CondBOp->getRHS(), TrueBlock, FalseBlock);

      // br(X && true) -> br(X)
      if (evaluator.EvaluateAsBooleanCondition(CondBOp->getRHS(), result))
        // br(X && true) -> br (X)
        return EmitBranchOnBoolExpr(CondBOp->getLHS(), TrueBlock, FalseBlock);

      // Emit the LHS as a conditional. If the LHS conditional is false, we want
      // to jump to the FalseBlock. Short-circuit evaluation
      // https://en.wikipedia.org/wiki/Short-circuit_evaluation
      //			extern int num;
      // e.g.		if (lhs > rhs && num == 0)	----> We can't evaluate
      // the "lhs > rhs" on the fly.
      //			{
      //----> if.then block
      //				...
      //			}
      //			else
      //			{
      //----> if.else block
      //				...
      //			}
      // At first, we generate code for "lhs > rhs", connecting "num == 0" for
      // true and "if.else" for false.
      BBPtr LHSTrue = CreateBasicBlock(getCurLocalName("and.lhs.true"));
      EmitBranchOnBoolExpr(CondBOp->getLHS(), LHSTrue, FalseBlock);
      EmitBlock(LHSTrue);

      print(LHSTrue);

      EmitBranchOnBoolExpr(CondBOp->getRHS(), TrueBlock, FalseBlock);
      return;
    } else if (CondBOp->getOpcode() == "||") {
      // If we have "false || X", simplify the code. "true || X" would have
      // constant folded if the case was simple enough.
      bool result;
      if (evaluator.EvaluateAsBooleanCondition(CondBOp->getLHS(), result))
        // br(false || X) -> br(X)
        return EmitBranchOnBoolExpr(CondBOp->getRHS(), TrueBlock, FalseBlock);
      if (evaluator.EvaluateAsBooleanCondition(CondBOp->getRHS(), result))
        // br(X || false) -> br(X)
        return EmitBranchOnBoolExpr(CondBOp->getLHS(), TrueBlock, FalseBlock);

      // Refer to "And", here also implement the short-circuit principle.
      // Emit the LHS as a conditional. If the LHS conditional is true, we want
      // to jump to the TrueBlock.
      BBPtr LHSFalse = CreateBasicBlock(getCurLocalName("or.lhs.false"));
      EmitBranchOnBoolExpr(CondBOp->getLHS(), TrueBlock, LHSFalse);
      EmitBlock(LHSFalse);

      print(LHSFalse);

      EmitBranchOnBoolExpr(CondBOp->getRHS(), TrueBlock, FalseBlock);
      return;
    }
  }

  // Deal with the situation of br(!X).
  if (UnaryPtr CondUOp = std::dynamic_pointer_cast<UnaryExpr>(Cond)) {
    if (CondUOp->getOpcode() == "!")
      return EmitBranchOnBoolExpr(CondUOp->getSubExpr(), FalseBlock, TrueBlock);
  }

  // Emit the code with the fully general case.
  ValPtr CondV = Cond->Accept(this);
  auto ret = CreateCondBr(CondV, TrueBlock, FalseBlock);
  print(ret);
}

ValPtr ModuleBuilder::EvaluateExprAsBool(ExprASTPtr E) { return 0; }

/// \brief Handle the algebraic and boolean operation, include '+' '-' '*' '/'
/// '%'
///	'<' '>' '<=' '>=' '==' '!='.
ValPtr ModuleBuilder::EmitAlgAndBooleanOp(const CGExpr::BinOpInfo &BInfo) {
  std::string Opcode = BInfo.BE->getOpcode();

  ValPtr ret = nullptr;

  if (Opcode == "+" || Opcode == "+=")
    ret = CreateAdd(BInfo.LHS, BInfo.RHS, getCurLocalName("add.tmp"));
  if (Opcode == "-" || Opcode == "-=")
    ret = CreateSub(BInfo.LHS, BInfo.RHS, getCurLocalName("sub.tmp"));
  if (Opcode == "*" || Opcode == "*=")
    ret = CreateMul(BInfo.LHS, BInfo.RHS, getCurLocalName("mul.tmp"));
  if (Opcode == "/" || Opcode == "/=")
    ret = CreateDiv(BInfo.LHS, BInfo.RHS, getCurLocalName("div.tmp"));
  if (Opcode == "%" || Opcode == "%=")
    ret = CreateRem(BInfo.LHS, BInfo.RHS, getCurLocalName("rem.tmp"));
  if (Opcode == "<")
    ret = CreateCmpLT(BInfo.LHS, BInfo.RHS, getCurLocalName("lt.tmp"));
  if (Opcode == ">")
    ret = CreateCmpGT(BInfo.LHS, BInfo.RHS, getCurLocalName("gt.result"));
  if (Opcode == "<=") {
    // return CreateCmpLE(BInfo.LHS, BInfo.RHS, "le.result");
    ret = CreateCmpLE(BInfo.LHS, BInfo.RHS, getCurLocalName("le.result"));
  }
  if (Opcode == ">=") {
    // return CreateCmpGE(BInfo.LHS, BInfo.RHS, "ge.result");
    ret = CreateCmpGE(BInfo.LHS, BInfo.RHS, getCurLocalName("ge.result"));
  }
  if (Opcode == "==") {
    // return CreateCmpEQ(BInfo.LHS, BInfo.RHS, "eq.result");
    ret = CreateCmpEQ(BInfo.LHS, BInfo.RHS, getCurLocalName("eq.result"));
  }
  if (Opcode == "!=") {
    // return CreateCmpNE(BInfo.LHS, BInfo.RHS, "ne.result");
    ret = CreateCmpNE(BInfo.LHS, BInfo.RHS, getCurLocalName("ne.result"));
  }

  print(ret);

  return ret;
  assert(0 && "Unreachable program point.");
  // An invalid LValue, the assert will ensure that this point is never reached.
  return nullptr;
}

/// \brief Handle the assign operation, include '='.
/// e.g.	mem = num;		---->	-----------------------------
///									| %tmp = load i32* %num
///| 									| store i32 %tmp, i32* %mem	|
///									-----------------------------
ValPtr ModuleBuilder::EmitBinAssignOp(const BinaryExpr *B) {
  LValue LHSAddr = EmitLValue(B->getLHS().get());

  // Aggregate
  auto ty = Types.ConvertType(B->getType());
  if (ty->isAggregateType()) {
    EmitAggExpr(B->getRHS().get(), LHSAddr.getAddress());
    return LHSAddr.getAddress();
  } else {
    ValPtr RHSV = B->getRHS()->Accept(this);
    // Store the value into the LHS.
    // --------------------Assignment operators-------------------------
    // An assignment operator stores a value in the object designated by the
    // left operand. An assignment expression has the value of the left operand
    // after the assignment, but is not an lvalue.
    // http://stackoverflow.com/questions/22616044/assignment-operator-sequencing-in-c11-expressions
    //---------------------Assignment operators-------------------------
    EmitStoreThroughLValue(RValue::get(RHSV), LHSAddr);
    return EmitLoadOfLValue(LHSAddr).getScalarVal();
  }
}

/// \brief Handle the compound assign operation, include '*=' '/=' '%=' '+='
/// '-='
///	'&&=' '||='.
///	e.g.		mem += num;		---->
///----------------------------- 										| %tmp = load i32* %num		| 										|
///%tmp1 = load i32* %mme	| 										| %add = i32 %tmp, i32 %tmp1|
///| 										| store i32 %add, i32* %mem	|
///										-------------------------
ValPtr ModuleBuilder::EmitCompoundAssignOp(const BinaryExpr *BE) {
  // (1) Emit the RHS first.
  ValPtr RHSV = BE->getRHS()->Accept(this);

  // (2) Load the LHS.
  LValue LHSLV = EmitLValue(BE->getLHS().get());
  ValPtr LHSV = EmitLoadOfLValue(LHSLV).getScalarVal();

  // (3) Perform the operation.
  CGExpr::BinOpInfo info;
  info.LHS = LHSV;
  info.RHS = RHSV;
  info.BE = BE;
  info.Ty = BE->getType();

  ValPtr ResultV = EmitAlgAndBooleanOp(info);

  // (4) Store the result value into the LHS lvalue.
  // Note: 'An assignment expression has the value of the left operand after the
  //		assignment...'
  // To Do: As a result of copying the Clang, 'EmitStoreThroughLValue()' and
  // 'EmitLoadOfLValue()' have a little difference(one need AST type and the
  // other not).
  EmitStoreThroughLValue(RValue::get(ResultV), LHSLV);

  return EmitLoadOfLValue(LHSLV).getScalarVal();
}

/// \brief Handle the binary expression.
ValPtr ModuleBuilder::EmitBinaryExpr(const BinaryExpr *B) {
  std::string Opcode = B->getOpcode();
  if (Opcode == "=")
    return EmitBinAssignOp(B);
  if (Opcode == "*=" || Opcode == "/=" || Opcode == "%=" || Opcode == "+=" ||
      Opcode == "-=" || Opcode == "&&=" || Opcode == "||=")
    return EmitCompoundAssignOp(B);

  CGExpr::BinOpInfo info;
  info.LHS = B->getLHS()->Accept(this);
  info.RHS = B->getRHS()->Accept(this);
  info.Ty = B->getType();
  info.BE = B;
  if (Opcode == "+" || Opcode == "-" || Opcode == "*" || Opcode == "/" ||
      Opcode == "%" || Opcode == "<" || Opcode == ">" || Opcode == "<=" ||
      Opcode == ">=" || Opcode == "==" || Opcode == "!=")
    return EmitAlgAndBooleanOp(info);

  assert(0 && "Unreachable program point.");
  return nullptr;
}

/// EmitLValue - Emit code to compute a designator that specifies the location
/// of the expression. For example, 'a = 10' or 'a.start.hieght = 10', we need
/// compute a location to store '10'.
LValue ModuleBuilder::EmitLValue(const Expr *E) {
  if (const DeclRefExpr *DRE = dynamic_cast<const DeclRefExpr *>(E)) {
    return EmitDeclRefLValue(DRE);
  }

  if (const MemberExpr *ME = dynamic_cast<const MemberExpr *>(E)) {
    return EmitMemberExprLValue(ME);
  }

  if (const CallExpr *CE = dynamic_cast<const CallExpr *>(E)) {
    // To Do:
  }
  assert(0 && "Unreachable program point.");
  return LValue();
}

/// \brief Get the decl address(Alloca Inst).
LValue ModuleBuilder::EmitDeclRefLValue(const DeclRefExpr *DRE) {
  // DeclRefExpr is still very simple. --- 2016-7-5
  std::string DeclName = DRE->getDeclName();
  auto Sym = CurScope->Resolve(DeclName);
  assert(Sym && "No alloca instruciton created for the decl?");
  if (std::shared_ptr<VariableSymbol> VarSym =
          std::dynamic_pointer_cast<VariableSymbol>(Sym)) {
    auto Alloca = VarSym->getAllocaInst();
    assert(Alloca && "No alloca instruciton created for the VarDecl?");
    return LValue::MakeAddr(Alloca);
  }
  if (std::shared_ptr<ParmDeclSymbol> ParmSym =
          std::dynamic_pointer_cast<ParmDeclSymbol>(Sym)) {
    auto Alloca = ParmSym->getAllocaInst();
    assert(Alloca && "No alloca instruciton created for the ParmDecl?");
    return LValue::MakeAddr(Alloca);
  }
  assert(0 && "Unreachable program point.");
  return LValue();
}

LValue ModuleBuilder::EmitMemberExprLValue(const MemberExpr *ME) {
  auto base = ME->getBase();
  // (1) Get the base address.
  LValue BaseLValue = EmitLValue(ME->getBase().get());

  // (2) According to specified member to get the offset.
  // e.g.	class B { var num : int; var mem : int; };
  //		class A { var num : int; var flag: bool; var b : B;};
  //		var a : A;
  // We have type size info, so we can get the offset easily.
  // a.num  ----> BaseAddr, int
  // a.flag ----> BaseAddr
  // a.b.mem ----> BaseAddr, int * 3
  auto idx = ME->getIdx();
  ValPtr MemberAddr =
      CreateGEP(Types.ConvertType(ME->getType()), BaseLValue.getAddress(), idx);
  print(MemberAddr);
  return LValue::MakeAddr(MemberAddr);
}

LValue ModuleBuilder::EmitCallExprLValue(const CallExpr *CE) {
  auto CallResult = EmitCallExpr(CE);
  return LValue::MakeAddr(nullptr);
}

/// \brief EmitLoadOfLValue - Given an expression with complex type that
/// represents a l-value, this method emits the address of the l-value, then
/// loads and returns the result.
ValPtr ModuleBuilder::EmitLoadOfLValue(const Expr *E) {
  return EmitLoadOfLValue(EmitLValue(E)).getScalarVal();
}

/// \brief EmitLoadOfLValue - Given an expression that represents a value
/// lvalue, this method emits the address of the lvalue, then loads the result
/// as an rvalue, returning the rvalue.
RValue ModuleBuilder::EmitLoadOfLValue(LValue LV) {
  ValPtr Ptr = LV.getAddress();
  auto V = CreateLoad(LV.getAddress());
  print(V);
  return RValue::get(V);
}

/// \brief EmitUnaryExpr - Emit code for unary expression.
ValPtr ModuleBuilder::EmitUnaryExpr(const UnaryExpr *UE) {
  std::string Opcode = UE->getOpcode();
  // Minus
  if (Opcode == "-") {
    ValPtr OperandV = UE->getSubExpr()->Accept(this);
    return CreateNeg(OperandV, "neg");
  }

  // We must distinguish pre between post.
  if (Opcode == "++") {
    if (UE->isLValue())
      return EmitPrePostIncDec(UE, true, true);
    else
      return EmitPrePostIncDec(UE, true, false);
  }
  if (Opcode == "--") {
    if (UE->isLValue())
      return EmitPrePostIncDec(UE, false, true);
    else
      return EmitPrePostIncDec(UE, false, false);
  }

  if (Opcode == "!") {
    ValPtr OperandV = UE->getSubExpr()->Accept(this);
    auto ret = CreateNot(OperandV, getCurLocalName("not"));
    print(ret);
    return ret;
  }
  return nullptr;
}

ValPtr ModuleBuilder::EmitMemberExpr(const MemberExpr *ME) { return nullptr; }

/// \brief EmitPrePostIncDec - Generate code for inc and dec.
/// e.g.  ++num(pre-inc)  ------>	  -----------------------------
///                                | %tmp = load i32* %num       |
///                                | %inc = add i32 %tmp, 1	     |  ----> the result
///                                | store i32 %inc, i32 * %num  |
///                                 -----------------------------
///			num++(post-inc) ------>     -----------------------------
///                                | %tmp1 = load i32* %num	     |  ----> the result
///                                | %inc2 = add i32 %tmp, 1     |
///                                | store i32 %inc2, i32* %num  |
///                                 -----------------------------
/// Note: Since only integer can be increase and the decrese.
ValPtr ModuleBuilder::EmitPrePostIncDec(const UnaryExpr *UE, bool isInc,
                                        bool isPre) {
  int AmoutVal = isInc ? 1 : -1;
  ValPtr NextVal;
  // (1) Get the sub expression's address.
  LValue LV = EmitLValue(UE->getSubExpr().get());
  compiler::IRBuild::ASTTyPtr ValTy = UE->getSubExpr()->getType();

  // (2) Get the sub expression's value.
  ValPtr InVal = EmitLoadOfLValue(LV).getScalarVal();

  // (3) Perform the operationn.
  NextVal = ConstantInt::get(Context, AmoutVal);
  NextVal->setName(std::to_string(AmoutVal));
  NextVal = CreateAdd(InVal, NextVal, getCurLocalName(isInc ? "inc" : "dec"));
  print(NextVal);

  // (4) Save the ResultValue.
  EmitStoreThroughLValue(RValue::get(NextVal), LV);
  // i.	++pre;		(1) Perform operation.
  // ii.	post++;		(2) Return the Val and perform operation.
  if (!isPre)
    return InVal;

  // There is no need to return the Result Value.
  return NextVal;
}

/// \brief EmitCallExpr - Emit the code for call expression.
///	e.g.	func add(lhs:int, rhs:int) -> int { return lhs + rhs;}
///			var a = 10;
///			var b = 101;
///			a = add(a, b);
///		(1) Emit code for argument.
///			%tmp = load i32* %a
///			%tmp1 = load i32* %b
///
///		(2) Emit code for call expression.
///			%call = call i32 @add(i32 %tmp, i32 %tmp1)
RValue ModuleBuilder::EmitCallExpr(const CallExpr *CE) {
  // Get the funciton decl's address.
  auto FD = CE->getFuncDecl();

  auto Sym = SymbolTree->Resolve(FD->getFDName());
  assert(Sym && "function decl symbol doesn't exists.");
  std::shared_ptr<FunctionSymbol> FuncSym =
      std::dynamic_pointer_cast<FunctionSymbol>(Sym);
  assert(FuncSym && "function decl symbol doesn't exists.");

  auto CalleeAddr = FuncSym->getFuncAddr();

  // If `FD` is builtin function, we should create an artificial `FuncAddr`.
  // Since there is no function declaration corresponding to `print`, so create
  // one.
  if (FD->isBuiltin()) {
    assert(!CalleeAddr && "Builtin function have no function declaration.");

    // generate function info.
    std::shared_ptr<CGFunctionInfo const> FI =
        Types.arrangeFunctionInfo(FD.get());

    auto TypeAndName = Types.getFunctionType(FD.get(), FI);

    CurFunc->CGFnInfo = FI;
    CurFunc->CurFuncDecl = const_cast<FunctionDecl *>(FD.get());
    CurFunc->FnRetTy = Types.ConvertType(FD->getReturnType());
    CalleeAddr = Function::create(TypeAndName.first, FD->getFDName(),
                                  TypeAndName.second);

    FuncSym->setFuncAddr(CalleeAddr);
  }

  return EmitCall(FD.get(), CalleeAddr, CE->getArgs());
}

/// \brief visit(const BinaryExpr*) - Generate code for BinaryExpr.
/// We need to make AggregateType case special treatment.
ValPtr ModuleBuilder::visit(const BinaryExpr *B) { return EmitBinaryExpr(B); }

ValPtr ModuleBuilder::visit(const DeclRefExpr *DRE) {
  auto ty = Types.ConvertType(DRE->getType());
  if (ty->isAggregateType())
    return EmitAggLoadOfLValue(DRE, nullptr);
  return EmitLoadOfLValue(DRE);
}

ValPtr ModuleBuilder::visit(const BoolLiteral *BL) {
  if (BL->getVal())
    return ConstantBool::getTrue(Context);
  else
    return ConstantBool::getFalse(Context);
}

ValPtr ModuleBuilder::visit(const NumberExpr *NE) {
  auto CInt = ConstantInt::get(Context, NE->getVal());
  CInt->setName(std::to_string(NE->getVal()));
  return CInt;
}

/// \brief Generate the code for UnaryExpr, include ''
/// Note: Post-increment and Pre-increment can be distinguished by
/// ExprValueKind.
//// ++x is lvalue and x++ is rvalue.
ValPtr ModuleBuilder::visit(const UnaryExpr *UE) { return EmitUnaryExpr(UE); }

ValPtr ModuleBuilder::visit(const MemberExpr *ME) {
  auto ty = Types.ConvertType(ME->getType());
  if (ty->isAggregateType())
    return EmitAggLoadOfLValue(ME, nullptr);
  return EmitLoadOfLValue(ME);
}

ValPtr ModuleBuilder::visit(const CallExpr *CE) {
  auto ty = Types.ConvertType(CE->getType());
  if (ty->isAggregateType())
    return EmitAggLoadOfLValue(CE, nullptr);
  return EmitCallExpr(CE).getScalarVal();
}

/// EmitScalarExpr - Emit the computation of the specified expression of
/// scalar type, returning the result.
ValPtr ModuleBuilder::EmitScalarExpr(const Expr *E) { return E->Accept(this); }

/// EmitStoreThroughLValue - Store the specified rvalue into the specified
/// lvalue, where both are guaranteed to the have the same type.
void ModuleBuilder::EmitStoreThroughLValue(RValue Src, LValue Dst,
                                           bool isInit) {
  EmitStoreOfScalar(Src.getScalarVal(), Dst.getAddress());
}

/// \brief Handle the scalar expression.
void ModuleBuilder::EmitStoreOfScalar(ValPtr Value, ValPtr Addr) {
  auto ret = CreateStore(Value, Addr);
  print(ret);
}