//===----------------------------IRBuilder.h-----------------------------===//
//
// This class for intermediate generation.
//
//===--------------------------------------------------------------------===//

// ---------------------------nonsense for coding---------------------------
// At some pounsigned in each of the code-generation "visit" methods, actual
// code must be emitted. The syntax and specification of such code depend on the
// form of moses IR.
// -------------------------------------------------------------------------

#pragma once
#include "IR/BasicBlock.h"
#include "IR/ConstantAndGlobal.h"
#include "IR/ConstantFolder.h"
#include "IR/Function.h"
#include "IR/Instruction.h"
#include "IR/MosesIRContext.h"
#include "IR/Value.h"
#include "IRBuild/CGCall.h"
#include "Parser/ast.h"
#include "Parser/constant-evaluator.h"
#include "Parser/sema.h"
#include "CGValue.h"
#include "CodeGenTypes.h"
#include <cassert>
#include <list>
#include <memory>

namespace IRBuild {
using namespace ast;
using namespace sema;
using namespace IR;
using namespace CodeGen;

using Opcode = BinaryOperator::Opcode;
using CallArgList = std::vector<std::pair<RValue, std::shared_ptr<ASTType>>>;

namespace CGStmt {
// BreakContinueStack - This keeps track of where break and continue
// statements should jump to.
struct BreakContinue {
  BreakContinue(std::shared_ptr<BasicBlock> bb, std::shared_ptr<BasicBlock> cb) : BreakBlock(bb), ContinueBlock(cb) {}
  std::shared_ptr<BasicBlock> BreakBlock;
  std::shared_ptr<BasicBlock> ContinueBlock;
};
} // namespace CGStmt

namespace CGExpr {
struct BinOpInfo {
  std::shared_ptr<Value> LHS;
  std::shared_ptr<Value> RHS;
  std::shared_ptr<ASTType> Ty;
  const BinaryExpr *BE;
};
} // namespace CGExpr

/// \brief FunctionBuilderStatus - hold the status when we are generating code
/// for Funciton.
struct FunctionBuilderStatus {
  FunctionDecl *CurFuncDecl;
  CGFunctionInfo CGFnInfo;
  TyPtr FnRetTy;
  std::shared_ptr<Function> CurFn;
  // Count the return expr(contain implicit return-expr).
  unsigned NumReturnExprs;

  // For switching context to top-level.
  std::shared_ptr<BasicBlock> TopLevelCurBB;
  std::shared_ptr<BasicBlock> TopLevelEntry;
  std::list<std::shared_ptr<Instruction>>::iterator TopLevelAllocaIsPt;
  std::list<std::shared_ptr<Instruction>>::iterator TopLevelCurIsPt;
  bool TopLevelIsAllocaInsertPointSetByNormalInsert;
  unsigned TopLevelTempCounter;

  /// ReturnBlock - Unified return block.
  /// ReturnBlock can omit later.
  std::shared_ptr<BasicBlock> ReturnBlock;
  /// ReturnValue - The temporary alloca to hold the return value. This is null
  /// iff the function has no return value.
  /// If we have multiple return statements, we need this tempprary alloca.
  /// e.g	    func add(parm : var)
  ///         {
  ///             int num = 10;
  ///             int mem = 20;
  ///				if (num > 10)
  ///					-----> we will load from num and store to the temp
  ///alloca. 					return num; 				else
  ///					-----> Just like 'return num', we need a temp alloca to hold
  ///the return value. 					return mem;
  ///			}
  ///	moses IR:
  ///			%1 = alloca i32 ----> temp alloca to hold the return
  ///value.
  ///			include.
  ///		; <label>:then
  ///			%6 = load i32* %num
  ///			store i32 %6, i32* %1
  ///			br <label> %9
  ///		; <label>:else
  ///			%7 = load i32* %mem
  ///			store i32 %7�� i32* %1
  ///			br <label> %8
  ///		; <label>:8
  ///			%9 = load i32* %1
  ///			ret i32 %9
  std::shared_ptr<Value> ReturnValue;

  explicit FunctionBuilderStatus()
      : CurFuncDecl(nullptr), CGFnInfo(), FnRetTy(nullptr),
        CurFn(nullptr), NumReturnExprs(0), TopLevelTempCounter(0),
        ReturnValue(nullptr) {}
};

/// \brief ModuleBuilder - This class for module code generation.
/// The outermost portions of an AST contain class and method declarations.
/// The ModuleBuilder is responsible for processing each class and method
/// declarations.
/// -------------------------------------------------------------------
class ModuleBuilder : public StatementAST::Visitor<std::shared_ptr<Value>> {
public:
  using FuncStatusPtr = std::shared_ptr<FunctionBuilderStatus>;
  static std::string LocalInstNamePrefix;

private:
  // Symbol table from sema.
  std::shared_ptr<Scope> SymbolTree;
  std::shared_ptr<Scope> CurScope;

  std::vector<CGStmt::BreakContinue> BreakContinueStack;

  // Auxiliary module.
  ConstantEvaluator evaluator;
  MosesIRContext &Context;
  CodeGenTypes Types;

  // IRs for the whole translation-unit.
  std::list<std::shared_ptr<Value>> IRs;

  std::shared_ptr<BasicBlock> CurBB;
  // CurFunc - Contains all the state information about current function.
  FuncStatusPtr CurFunc;

  // 'InsertPoint' is pointing at the end of current basic block of most of the
  // time. Note we should clear 'InsertPoint' when we switch to the new block.
  std::list<std::shared_ptr<Instruction>>::iterator InsertPoint;

  // The insert point for alloca instructions.
  // Alloca instruction only appear in the Entry Block.
  // e.g:		define i32 add(i32 parm)
  //			{
  //                         --------------------
  //                        |	    AllocaInst     | ----> Entry Block
  //                        |	    AllocaInst     |
  //                        |	    Other Inst     | ----> Alloca insert point.
  //                         --------------------
  // Before executing the function code, we should pre-allocate a portion of
  // memory on the stack.
  std::list<std::shared_ptr<Instruction>>::iterator AllocaInsertPoint;
  bool isAllocaInsertPointSetByNormalInsert;
  std::shared_ptr<BasicBlock> EntryBlock;
  // This is just for naming the temporary result(the name of the instruction.).
  unsigned TempCounter;

public:
  ModuleBuilder(std::shared_ptr<Scope> SymbolInfo, MosesIRContext &context);

  /// \brief This method is used for get MosesIRContext.
  MosesIRContext &getMosesIRContext() { return Context; }

  /// \brief Get the current instruction'a name. Return the "%" + Name +
  /// TempCounter. e.g.	Name = "num"
  ///	Return "%num0"
  std::string getCurLocalName(std::string Name = "") {
    return LocalInstNamePrefix + Name + std::to_string(TempCounter++);
  }

  void VisitChildren(std::vector<std::shared_ptr<StatementAST>> AST);

  // Note: We only need to consider the type of AST leaf node.
  std::shared_ptr<Value> visit([[maybe_unused]] const StatementAST *stmt) { return nullptr; }
  std::shared_ptr<Value> visit([[maybe_unused]] const ExprStatement *exprstmt) { return nullptr; }
  std::shared_ptr<Value> visit(const CompoundStmt *comstmt);
  std::shared_ptr<Value> visit(const IfStatement *ifstmt);
  std::shared_ptr<Value> visit(const WhileStatement *whilestmt);
  std::shared_ptr<Value> visit(const ReturnStatement *retstmt);
  std::shared_ptr<Value> visit([[maybe_unused]] const DeclStatement *declstmt) { return nullptr; }
  std::shared_ptr<Value> visit(const BreakStatement *BS);
  std::shared_ptr<Value> visit(const ContinueStatement *CS);
  std::shared_ptr<Value> visit(const VarDecl *VD);
  std::shared_ptr<Value> visit([[maybe_unused]] const ParameterDecl *PD) { return nullptr; }
  std::shared_ptr<Value> visit([[maybe_unused]] const ClassDecl *CD) { return nullptr; }
  std::shared_ptr<Value> visit(const FunctionDecl *FD);
  std::shared_ptr<Value> visit([[maybe_unused]] const UnpackDecl *UD);
  std::shared_ptr<Value> visit(const BinaryExpr *B);
  std::shared_ptr<Value> visit(const CallExpr *Call);
  std::shared_ptr<Value> visit(const DeclRefExpr *DRE);
  std::shared_ptr<Value> visit(const BoolLiteral *BL);
  std::shared_ptr<Value> visit(const NumberExpr *NE);
  std::shared_ptr<Value> visit(const UnaryExpr *UE);
  std::shared_ptr<Value> visit(const MemberExpr *ME);
  std::shared_ptr<Value> visit([[maybe_unused]] const Expr *E) { return nullptr; }

  const std::list<std::shared_ptr<Value>> &getIRs() const { return IRs; };

private:
  //===-----------------------------------------------------------===//
  // Helper for variable declaration generation.
  //===-----------------------------------------------------------===//
  class VarEmission {
    bool wasEmittedAsGlobal() const { return true; }

  public:
  };

  /// EmitVarDecl - This method handles emission of any variable declaration
  /// inside a function.
  void EmitLocalVarDecl(const VarDecl *var);

  /// \brief Emit a branch from the current block to the target one if this
  /// was a real block.
  void EmitBrach(std::shared_ptr<BasicBlock> Target);

  /// EmitParmDecl - Emit an alloca (or GlobalValue depending on target)
  /// for the specified parameter and set up LocalDeclMap.
  void EmitParmDecl(const VarDecl *VD, std::shared_ptr<Value> Arg);

  /// EmitLocalVarAlloca - Emit the alloca.
  std::shared_ptr<AllocaInst>  EmitLocalVarAlloca(const VarDecl *var);
  //===--------------------------------------------------------------===//
  // Helper for function call generation.
  //===--------------------------------------------------------------===//
  /// ExpandTypeToArgs - Expand an RValue \arg Src, with the IR type for
  /// \arg Ty, into individual arguments on the provided vector \arg Args.
  /// ----------------------clang2.6 Expand--------------------
  /// Expand - only valid for aggregate argument types. The structure should be
  /// expanded into consecutive arguments for its constituent fields. Currently
  /// expand is only allowed on structures whose fields are all scalar types or
  /// are all scalar types or are themselves expandable types.
  /// ----------------------clang2.6 Expand--------------------
  void ExpandTypeToArgs(std::shared_ptr<ASTType> ASTTy, RValue Src, std::vector<std::shared_ptr<Value>> &Args);

  /// ExpandTypeFromArgs - Reconstruct a structure of type \arg Ty
  /// from function arguments into \arg Dst.
  void ExpandTypeFromArgs(std::shared_ptr<ASTType> ASTTy, LValue LV,
                          std::vector<std::shared_ptr<Value>> &SubArgs);

  RValue EmitCall(const FunctionDecl *FD, std::shared_ptr<Value> FuncAddr,
                  const std::vector<ExprASTPtr> &Args);

  RValue EmitCall(const FunctionDecl *FD, CGFunctionInfo CGFunInfo,
                  std::shared_ptr<Value> FuncAddr, CallArgList CallArgs);

  /// EmitCallArg - Emit a single call argument.
  void EmitCallArg(const Expr *E, std::shared_ptr<ASTType> ArgType);

  /// EmitCallArgs - Emit call arguments for a function.
  void EmitCallArgs(CallArgList &CallArgs,
                    const std::vector<ExprASTPtr> &ArgExprs);

  /// EmitBranchOnBoolExpr - Emit a branch on a boolean condition(e.g for an
  /// if statement) to the specified blocks. Based on the condition, this might
  /// try to simplify the codegen of the conditional based on the branch.
  /// To Do: TrueCount for PGO(profile-guided optimization).
  void EmitBranchOnBoolExpr(ExprASTPtr Cond, std::shared_ptr<BasicBlock> TrueB,
                            std::shared_ptr<BasicBlock> FalseBlock /*, unsigned TrueCount*/);

  //===---------------------------------------------------------===//
  // Helper for builder configuration.
  //===---------------------------------------------------------===//
  /// \brief Insert a new instruction to the current block.
  template <typename InstTy>
  InstTy InsertHelper(InstTy I, std::string Name = "%") {
    if (Name == "%") {
      Name = getCurLocalName();
    }
    if (!isAllocaInsertPointSetByNormalInsert) {
      AllocaInsertPoint = CurBB->Insert(InsertPoint, I);
      isAllocaInsertPointSetByNormalInsert = true;
    } else {
      CurBB->Push(I);
    }

    // CurBB->Push(I);
    I->setName(Name);
    return I;
  }

  std::shared_ptr<ConstantBool> InsertHelper(std::shared_ptr<ConstantBool> B, std::string Name = "") {
    B->setName(Name);
    return B;
  }

  std::shared_ptr<ConstantInt> InsertHelper(std::shared_ptr<ConstantInt> I, std::string Name = "") {
    I->setName(Name);
    return I;
  }

  std::shared_ptr<AllocaInst> InsertHelper(std::shared_ptr<AllocaInst>  I, std::string Name = "") {
    if (EntryBlock) {
      if (Name == "") {
        Name = getCurLocalName();
      }
      EntryBlock->Insert(AllocaInsertPoint, I);
      I->setName(Name);
      return I;
    }
    assert(0 && "Unreachable code!");
  }

  /// \brief Template specialization.
  /*std::shared_ptr<Value> InsertHelper(std::shared_ptr<ConstantBool> V, std::string Name = "") const { return
     V; } std::shared_ptr<Value> InsertHelper(std::shared_ptr<ConstantInt> V, std::string Name = "") const {
     return V; }*/
  /// \brief Clear the insertion point: created instructions will not be
  /// inserted into a block.
  void ClearInsertionPoint() {
    CurBB = nullptr;
    // Note: We can't make 'InsertPoint' invalid or assign nullptr.
    // To Do: When we leave a BB, we need make 'InsertPoint' invalid.
    // InsertPoint = nullptr;
  }

  /// \brief This specifies that created instructions should be appended to the
  /// end of the ** specified block **.
  void SetInsertPoint(std::shared_ptr<BasicBlock> TheBB);

  /// \brief This specifies that created instructions should be inserted before
  /// the specified instruction.
  void SetInsertPoint(std::shared_ptr<Instruction> I);

  bool HaveInsertPoint() const { return CurBB ? true : false; }
  //===---------------------------------------------------------------===//
  // Miscellaneous creation methods.
  //===---------------------------------------------------------------===//

  /// \brief Get the constant value for i1 true.
  std::shared_ptr<ConstantBool> getTrue() { return ConstantBool::getTrue(Context); }
  /// \brief Get the constant value for i1 false;
  std::shared_ptr<ConstantBool> getFalse() { return ConstantBool::getFalse(Context); }
  /// \brief Get the constant value for int.
  std::shared_ptr<ConstantInt> getInt(int val) { return ConstantInt::get(Context, val); }

  //===---------------------------------------------------------===//
  // Instruction creation methods: Terminators
  //===---------------------------------------------------------===//
  /// \brief Create a 'ret void' instruction.
  std::shared_ptr<ReturnInst> CreateRetVoid();
  /// \brief Create a 'ret <val>' instruction.
  std::shared_ptr<ReturnInst> CreateRet(std::shared_ptr<Value>);

  std::shared_ptr<ReturnInst> CreateAggregateRet(std::vector<std::shared_ptr<Value>> retVals, unsigned N);

  /// \brief Create an unconditional 'br label X' instruction.
  std::shared_ptr<BranchInst> Create(std::shared_ptr<BasicBlock> Dest);

  /// \brief Create a conditional 'br Cond, TrueDest, FalseDest' instruction.
  /// MDNode *BranchWeights
  /// MDNode *Unpredictable
  std::shared_ptr<BranchInst> CreateCondBr(std::shared_ptr<Value> Cond, std::shared_ptr<BasicBlock> True, std::shared_ptr<BasicBlock> False);

  /// \brief Create an unconditional 'br label X' instruction.
  std::shared_ptr<BranchInst> CreateBr(std::shared_ptr<BasicBlock> Dest);

  //===------------------------------------------------------------------===//
  // Instruction creation methods: Binary Operators
  //===------------------------------------------------------------------===//

  std::shared_ptr<BinaryOperator> CreateInsertBinOp(BinaryOperator::Opcode Opc, std::shared_ptr<Value> LHS,
                              std::shared_ptr<Value> RHS, std::string Name);
  std::shared_ptr<Value> CreateAdd(std::shared_ptr<Value> LHS, std::shared_ptr<Value> RHS, std::string Name = "");
  std::shared_ptr<Value> CreateSub(std::shared_ptr<Value> LHS, std::shared_ptr<Value> RHS, std::string Name = "");
  std::shared_ptr<Value> CreateMul(std::shared_ptr<Value> LHS, std::shared_ptr<Value> RHS, std::string Name = "");
  std::shared_ptr<Value> CreateDiv(std::shared_ptr<Value> LHS, std::shared_ptr<Value> RHS, std::string Name = "");
  std::shared_ptr<Value> CreateRem(std::shared_ptr<Value> LHS, std::shared_ptr<Value> RHS, std::string Name = "");
  std::shared_ptr<Value> CreateShl(std::shared_ptr<Value> LHS, std::shared_ptr<Value> RHS, std::string Name = "");
  std::shared_ptr<Value> CreateShr(std::shared_ptr<Value> LHS, std::shared_ptr<Value> RHS, std::string Name = "");
  std::shared_ptr<Value> CreateAnd(std::shared_ptr<Value> LHS, std::shared_ptr<Value> RHS, std::string Name = "");
  std::shared_ptr<Value> CreateOr(std::shared_ptr<Value> LHS, std::shared_ptr<Value> RHS, std::string Name = "");
  std::shared_ptr<Value> CreateNeg(std::shared_ptr<Value> V, std::string Name = "");
  std::shared_ptr<Value> CreateNot(std::shared_ptr<Value> V, std::string Name = "");

  //===------------------------------------------------------------------===//
  // Instruction creation methods: Memory Instructions.
  //===------------------------------------------------------------------===//
  std::shared_ptr<AllocaInst> CreateAlloca(TyPtr Ty, std::string Name = "");
  std::shared_ptr<LoadInst> CreateLoad(std::shared_ptr<Value> Ptr);
  std::shared_ptr<StoreInst> CreateStore(std::shared_ptr<Value> Val, std::shared_ptr<Value> Ptr);
  std::shared_ptr<GetElementPtrInst> CreateGEP(TyPtr Ty, std::shared_ptr<Value> Ptr, std::vector<unsigned> IdxList,
                       std::string Name = "");
  std::shared_ptr<GetElementPtrInst> CreateGEP(TyPtr Ty, std::shared_ptr<Value> Ptr, unsigned Idx,
                       std::string Name = "");

  //===---------------------------------------------------------------===//
  // Instruction creation methods: Compare Instructions.
  //===---------------------------------------------------------------===//
  std::shared_ptr<Value> CreateCmpEQ(std::shared_ptr<Value> LHS, std::shared_ptr<Value> RHS, std::string Name = "");
  std::shared_ptr<Value> CreateCmpNE(std::shared_ptr<Value> LHS, std::shared_ptr<Value> RHS, std::string Name = "");
  std::shared_ptr<Value> CreateCmpGT(std::shared_ptr<Value> LHS, std::shared_ptr<Value> RHS, std::string Name = "");
  std::shared_ptr<Value> CreateCmpGE(std::shared_ptr<Value> LHS, std::shared_ptr<Value> RHS, std::string Name = "");
  std::shared_ptr<Value> CreateCmpLT(std::shared_ptr<Value> LHS, std::shared_ptr<Value> RHS, std::string Name = "");
  std::shared_ptr<Value> CreateCmpLE(std::shared_ptr<Value> LHS, std::shared_ptr<Value> RHS, std::string Name = "");
  std::shared_ptr<Value> CreateCmp(CmpInst::Predicate P, std::shared_ptr<Value> LHS, std::shared_ptr<Value> RHS,
                   std::string Name = "");

  //===---------------------------------------------------------------===//
  // Instruction creation methods: Other Instructions.
  //===---------------------------------------------------------------===//
  std::shared_ptr<PHINode> CreatePHI(TyPtr Ty, unsigned NumReservedValues,
                       std::string Name = "");
  std::shared_ptr<CallInst> CreateCall(std::shared_ptr<Value> Callee, std::vector<std::shared_ptr<Value>> Args);
  std::shared_ptr<CallInst> CreateIntrinsic(std::shared_ptr<Intrinsic> Intr, std::vector<std::shared_ptr<Value>> Args);
  std::shared_ptr<ExtractValueInst> CreateExtractValueValue(std::shared_ptr<Value> Agg, std::vector<unsigned> Idxs,
                                    std::string Name = "");

  //===----------------------------------------------------------------------===//
  // Utility creattion methods
  //===----------------------------------------------------------------------===//
  /// \brief Return an i1 value testing if \p Arg is null
  std::shared_ptr<Value> CreateIsNull(std::shared_ptr<Value> Arg, std::string Name = "");
  /// \brief Return an i1 value testing if \p Arg is not null.
  std::shared_ptr<Value> CreateIsNotNull(std::shared_ptr<Value> Arg, std::string Name = "");

  //===----------------------------------------------------------------------===//
  // Other create*().
  //===----------------------------------------------------------------------===//
  std::shared_ptr<BasicBlock> CreateBasicBlock(std::string Name, std::shared_ptr<Function> parent = nullptr,
                         std::shared_ptr<BasicBlock> before = nullptr);

private:
  //===---------------------------------------------------------===//
  // Helpers.
  //===---------------------------------------------------------===//
  void SaveTopLevelCtxInfo();
  void RestoreTopLevelCtxInfo();

private:
  //===---------------------------------------------------------===//
  /// \brief Emit code for function decl.
  void EmitFunctionDecl(const FunctionDecl *FD);

  /// \brief Handle the start of function.
  void StartFunction(CGFunctionInfo FnInfo, std::shared_ptr<Function> Fn);

  /// \brief Complete IR generation of the current function. It is legal to call
  /// this function even if there is no current insertion point.
  void FinishFunction();

  // EmitFunctionPrologue - Mainly for transferring formal parameters.
  // https://en.wikipedia.org/wiki/Function_prologue
  // -------------------------nonsense for coding------------------------
  // The function prologue is a few lines of code at the beginning of a
  // function, which prepare the 'stack' and 'registers' for use within
  // the funciton.
  // - Pushes the old base pointer onto the stack, such it can be restored
  // later.
  // - Assign the value of stack pointer(which is pointed to the saved base
  // pointer
  //	 and the top of the old stack frame) into base pointer such that a new
  //stack
  // frame will be created on top of the old stack frame(i.e. the top of the old
  //	stack frame will become the base of the new stack frame).
  // - Move the stack pointer further by decreasing or increasing its value,
  // depending
  //  on the whether the stack grows down or up. On x86, the stack pointer is
  //  decreased to make room for variables(i.e. the functions's local
  //  variables).
  // e.g.     ======================
  //         |     pushl %ebp       |
  //          ======================
  //         |     movl %esp, %ebp  |
  //          ======================
  //         |     subl $N, %esp    |
  //          ======================
  void EmitFunctionPrologue(CGFunctionInfo FunInfo,
                            std::shared_ptr<Function> fun);

  // EmitFunctionEpilogue - Reverse the actions of the function prologue and
  // returns control to the calling funciton. It typically does the following
  // actions (this procedure may differ from one architecture to another):
  // - Replaces the stack pointer with the current base (or frame) pointer, so
  // the
  //   stack pointer is restored to its value before the prologue
  // - Pops the base pointer off the stack, so it is restored to its value
  // before the
  //   prologue.
  // - Returns to calling function(caller), by popping the previous frame's
  // program
  //   counter off the stack and jumping to it.
  // e.g.         ======================
  //             |   movl %ebp, %esp    |
  //              ======================
  //             |   popl %ebp          |
  //              ======================
  //             |   ret                |
  //              ======================
  void EmitFunctionEpilogue(CGFunctionInfo CGFnInfo);
  //===---------------------------------------------------------===//
  // Emit code for stmts.

  void EmitFunctionBody(StmtASTPtr body);

  /// \brief Emit the unified return block, trying to avoid its emission when
  /// possible.
  void EmitReturnBlock();

  /// EmitCompoundStmt - Emit code for compound statements.
  /// e.g.	{
  ///				stmts.
  ///			}
  /// FixMe: I don't know why we need to return a value. But Clang
  /// 'EmitCompoundStmt'
  ///	need a value to return.
  std::shared_ptr<Value> EmitCompoundStmt(const StatementAST *S);
  void EmitIfStmt(const IfStatement *IS);
  void EmitConBrHints();
  void EmitWhileStmt(const WhileStatement *WS);
  void EmitBreakStmt([[maybe_unused]] const BreakStatement *BS);
  void EmitContinueStmt([[maybe_unused]] const ContinueStatement *CS);

  /// \brief EmitReturnStmt - Generate code for return statement.
  /// ----------------Clang/CGStmt.cpp/EmitReturnStmt()--------------
  /// Note that due to GCC extensions, this can have an operand if
  /// the function returns void, or may be missing one if the function
  /// returns non-void. Fun Stuff :).
  /// ----------------------------------------------------------------
  /// http://stackoverflow.com/questions/35987493/return-void-type-in-c-and-c
  /// This topic, discussed whether the ret-void function's return statement
  /// can have sub-expression.
  /// e.g.	void func1() { include. }
  ///			void func()
  ///			{
  ///				return func1();		----> is this correct?
  ///			}
  ///
  /// c11, 6.8.6.4 "The return statement:"
  ///	-	A return statement with an expression shall not appear in a
  ///function 		whose return type is void.
  /// -	return without expression not permitted in function that returns a
  ///		value (and vice versa)
  /// So this is forbidden in Standard C.
  ///
  /// However, this is allowed in C++.
  ///	C++14, 6.6.3 "The return statement:"
  ///	-	A return statement with an expression of non-void type can be used
  ///only 		in functions returning a value[include.] A return statement with an
  ///expression 		of type void can be only in functions with a return type of cv
  ///void; the 		expression is evaluated just the function returns its caller.
  ///
  /// Yes, you may use an expression if it is of void type (that's been valid
  /// since C++98). "Why would anyone want to write such nonsense code though?
  /// Rather than just writing
  ///	void f2() {f();}", so funny!

  std::shared_ptr<Value> EmitReturnStmt(const ReturnStatement *S);
  void EmitDeclStmt(const DeclStatement *S);

  //===---------------------------------------------------------===//
  // Emit code for expressions.

  /// EmitLValue() - Emit code to compute a designator that specifies the
  /// location of the expression. This function just return a simple address.
  /// For example:
  ///		var num : int;
  ///		num = 10;
  ///		~~~
  /// We should emit code for 'num'.
  LValue EmitLValue(const Expr *E);

  LValue EmitDeclRefLValue(const DeclRefExpr *DRE);

  LValue EmitMemberExprLValue(const MemberExpr *ME);

  LValue EmitCallExprLValue(const CallExpr *CE);

  /// EmitDeclRefLValue - Emit code to compute the location of DeclRefExpr.
  /// For example:
  ///		num = mem;
  ///		~~~
  LValue EmitDeclRefExpr(const DeclRefExpr *E);

  /// \brief EmitExprAsInit - Emit the code necessary to initialize a location
  /// in memory with the given initializer.
  std::shared_ptr<Value> EmitBinaryExpr(const BinaryExpr *BE);

  /// \brief EmitCallExpr - Emit the code for call expression.
  RValue EmitCallExpr(const CallExpr *CE);

  /// \brief EmitUnaryExpr - Emit code for unary expression, including '-' '!'
  /// '--' '++'.
  std::shared_ptr<Value> EmitUnaryExpr(const UnaryExpr *UE);

  /// \brief EmitPrePostIncDec - Emit code for '--' '++'.
  std::shared_ptr<Value> EmitPrePostIncDec(const UnaryExpr *UE, bool isInc, bool isPre);

  std::shared_ptr<Value> EmitMemberExpr([[maybe_unused]] const MemberExpr *ME);

  /// \brief Handle the algebraic and boolean operation, include '+' '-' '*' '/'
  /// '%'
  ///	'<' '>' '<=' '>=' '==' '!='.
  std::shared_ptr<Value> EmitAlgAndBooleanOp(const CGExpr::BinOpInfo &BE);

  /// \brief Handle the assign operation, include '='.
  std::shared_ptr<Value> EmitBinAssignOp(const BinaryExpr *BE);

  /// \brief Handle the compound assign operation, include '*=' '/=' '%=' '+='
  /// '-='
  ///	'&&=' '||='.
  std::shared_ptr<Value> EmitCompoundAssignOp(const BinaryExpr *BE);

  void EmitExprAsInit(const Expr *init, const VarDecl *D, LValue lvalue);
  void EmitScalarInit(const Expr *init, const VarDecl *D, LValue lvalue);

  /// \brief EmitLoadOfLValue - Given an expression with complex type that
  /// represents a l-value, this method emits the address of the l-value, then
  /// loads and returns the result.
  std::shared_ptr<Value> EmitLoadOfLValue(const Expr *E);

  /// \brief EmitLoadOfLValue - Given an expression that represents a value
  /// lvalue, this method emits the address of the lvalue, then loads the result
  /// as an rvalue, returning the rvalue.
  RValue EmitLoadOfLValue(LValue LV);

  /// EmitStoreOfScalar - Store a scalar value to an address.
  void EmitStoreOfScalar(std::shared_ptr<Value> V, std::shared_ptr<Value> Addr);

  /// EmitScalarExpr - Emit the computation of the specified expression of
  /// scalar type, returning the result.
  std::shared_ptr<Value> EmitScalarExpr(const Expr *E);

  /// EmitStoreThroughLValue - Store the specified rvalue into the specified
  /// lvalue.
  ///	For Example:
  ///		num = mem.	--------> EmitBinaryExpr()
  ///		   ~~~
  ///
  /// At the end of the 'EmitBinaryExpr()', we need handle the assignment(
  ///	load from mem and store to the num).
  void EmitStoreThroughLValue(RValue Src, LValue Dst, [[maybe_unused]] bool isInit = false);

  //===-----------------------------------------------------------------------===//
  // Aggregate Expr stuff.
  // Aggregate Type
  // (1) EmitFunctionProglog()  EmitCallExpr()
  // (2) EmitReturnStmt()
  // (3) EmitBinaryExpr()

  /// EmitAggExpr - Emit the computation of the specified expression of
  /// aggregate type. The result is computed into DestPtr. Note that if DestPtr
  /// is null, the value of the aggregate expression is not needed.
  void EmitAggExpr(const Expr *E, std::shared_ptr<Value> DestPtr);

  /// CreateCoercedStore - Create a store to \arg DstPtr from \arg Src,
  /// where the souece and destination may have different types.
  void CreateCoercedStore(std::shared_ptr<Value> Src, std::shared_ptr<Value> DestPtr);

  void EmitDeclRefExprAgg(const DeclRefExpr *DRE, std::shared_ptr<Value> DestPtr);
  void EmitMemberExprAgg(const MemberExpr *ME, std::shared_ptr<Value> DestPtr);
  void EmitCallExprAgg(const CallExpr *CE, std::shared_ptr<Value> DestPtr);
  void EmitBinaryExprAgg(const BinaryExpr *B);

  /// EmitAggregateCopy - Emit an aggregate copy.
  void EmitAggregateCopy(std::shared_ptr<Value> DestPtr, std::shared_ptr<Value> SrcPtr, std::shared_ptr<ASTType> Ty);

  /// EmitAggregateClear
  void EmitAggregateClear(std::shared_ptr<Value> DestPtr, std::shared_ptr<ASTType> Ty);

  // EmitAggLoadOfLValue - Given an expression with aggregate type that
  // represents a value lvalue, thid method emits the address of the lvalue,
  // then loads the result into DestPtr.
  std::shared_ptr<Value> EmitAggLoadOfLValue(const Expr *E, std::shared_ptr<Value> DestPtr);

  /// EmitFinalDestCopy - Perform the final copy to DestPtr, if desired.
  void EmitFinalDestCopy(const Expr *E, RValue Src, std::shared_ptr<Value> DestPtr);

  /// EmitFinalDestCopy - Perform the final copy to DestPtr, if desired.
  void EmitFinalDestCopy(const Expr *E, LValue Src, std::shared_ptr<Value> DestPtr);

  /// SimplifyForwardingBlocks - If the given basic block is only a branch to
  /// another basic block, simplify it. This assumes that no other code could
  /// potentially reference the basic block.
  /// e.g.            ----------------  BB1
  ///                |    include.    |
  ///                |                |
  ///                 ----/-----------\
	///                    /             \
	///        -----------/----  BB2     -\-----------  BB3
  ///       | br label %BB4  |        |             |
  ///        --------\-------          -------------
  ///                 \
	///            ------\---------  BB4
  ///           |                |
  ///            ----------------
  /// Note: BB2 is useless, we can eliminate it and replace all use of BB2 to
  /// BB4. As shown below. e.g.
  ///             ----------------  BB1
  ///            |   include.     |
  ///            |                |
  ///             ----/-----------\
	///                /             \
	///    -----------/----  BB4     -\-----------  BB3
  ///   |                |        |             |
  ///    ----------------          -------------
  void SimplifyForwardingBlocks(std::shared_ptr<BasicBlock> BB);

  /// EmitBlock - Emit the given block \arg BB and set it as the insert point,
  /// adding a fall-through branch from current insert block if necessary.
  void EmitBlock(std::shared_ptr<BasicBlock> BB, bool IsFinished = false);

  /// EvaluateExprAsBool = Perform the usual unary conversions on the specified
  /// expression and compare the result against zero, returning an Int1Ty value.
  std::shared_ptr<Value> EvaluateExprAsBool(ExprASTPtr E);
};
} // namespace IRBuild
