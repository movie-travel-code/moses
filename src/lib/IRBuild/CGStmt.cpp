//===-------------------------------CGStmt.cpp----------------------------===//
//
// This contains code to emit Stmt nodes as LLVM code.
//
//===---------------------------------------------------------------------===//
#include "IRBuild/IRBuilder.h"

using namespace IR;
using namespace IRBuild;
extern void print(std::shared_ptr<IR::Value> V);
std::shared_ptr<Value> ModuleBuilder::visit(const IfStatement *IS) {
  EmitIfStmt(IS);
  return nullptr;
}

std::shared_ptr<Value> ModuleBuilder::visit(const BreakStatement *BS) {
  EmitBreakStmt(BS);
  return nullptr;
}

std::shared_ptr<Value> ModuleBuilder::visit(const ContinueStatement *CS) {
  EmitContinueStmt(CS);
  return nullptr;
}

void ModuleBuilder::EmitBreakStmt([[maybe_unused]] const BreakStatement *BS) {
  assert(!BreakContinueStack.empty() && "break statement not in a loop!");
  std::shared_ptr<BasicBlock> Block = BreakContinueStack.back().BreakBlock;
  EmitBrach(Block);
}

void ModuleBuilder::EmitContinueStmt(
    [[maybe_unused]] const ContinueStatement *CS) {
  assert(!BreakContinueStack.empty() && "continue statement not in a loop!");
  std::shared_ptr<BasicBlock> Block = BreakContinueStack.back().ContinueBlock;
  EmitBrach(Block);
}

/// EmitBlock - Emit the given block \arg BB and set it as the insert point.
void ModuleBuilder::EmitBlock(std::shared_ptr<BasicBlock> BB, bool IsFinished) {
  /// Fall out of the current block (if necessary).
  /// e.g.     B0
  ///         /
  ///        /
  ///       B1
  /// We can merge the B0 and B1.
  EmitBrach(BB);
  if (IsFinished && BB->use_empty()) {
    BB.reset();
    return;
  }

  if (CurFunc->CurFn) {
    CurFunc->CurFn->addBB(BB);
  } else {
    IRs.push_back(BB);
  }
  // CurFunc->CurFn->getBasicBlockList().push_back(BB);
  SetInsertPoint(BB);
}

/// \brief Emit a branch from the current block to the target one if this
/// was a real block.
void ModuleBuilder::EmitBrach(std::shared_ptr<BasicBlock> Target) {
  if (!CurBB || CurBB->getTerminator()) {
    // If there is no insert point or the previous block is already
    // terminated, don't touch it.
  } else {
    // Otherwise, create a fall-through branch.
    auto ret = CreateBr(Target);
  }
  ClearInsertionPoint();
}

void ModuleBuilder::EmitReturnBlock() {
  if (CurFunc->ReturnBlock->use_empty())
    return;
  // otherwise, if the return block is the target of a single direct
  // branch then we can just put the code in that block instead.
  if (CurFunc->ReturnBlock->hasOneUse()) {
    const BranchInst *BI =
        dynamic_cast<const BranchInst *>(CurFunc->ReturnBlock->use_begin());
    if (BI && BI->isUncoditional() &&
        BI->getSuccessor(0).get() == CurFunc->ReturnBlock.get()) {
      auto BB = BI->getParent();
      SetInsertPoint(BB);
      BB->RemoveInst(BI);
      return;
    }
  }

  EmitBlock(CurFunc->ReturnBlock);
}

/// \brief EmitWhileStmt - Emit the code for while statement.
/// e.g.    while (num < lhs)    -------------------------  ---> Pre-Block
///         {                   |	          ...           |
///            lhs += 2;        |  br label %while.cond   |
///         }				             -------------------------
///
///                              -----------------------------------------------
///             ---> while.cond | %tmp = load i32* %num |
///                             |  %tmp1 = load i32* %lhs | | %cmp = cmp lt i32
///                             %tmp, %tmp1                 | | br %cmp, label
///                             %while.body, label %while.end  |
///                             -------------------------------------------------
///
///                             ----------------------------- ---> while.body
///                             | %tmp2 = load i32* %lhs	  |
///                             | %add = add i32 %tmp2, 2	  |
///                             | store i32 %add, i32* %lhs	|
///                             | br label %while.cond		  |
///                             -----------------------------
///
///                             ----------------------------- ---> while.end
///                             |            ...            |
///                             -----------------------------
void ModuleBuilder::EmitWhileStmt(const WhileStatement *whilestmt) {
  // Emit the header for the loop, insert it in current 'function',
  // which will create an uncond br to it.
  std::shared_ptr<BasicBlock> LoopHeader =
      CreateBasicBlock(getCurLocalName("while.cond"));
  EmitBlock(LoopHeader);

  // Create an exit block for when the condition fails, create a block
  // for the body of the loop.
  std::shared_ptr<BasicBlock> ExitBlock =
      CreateBasicBlock(getCurLocalName("while.end"));
  std::shared_ptr<BasicBlock> LoopBody =
      CreateBasicBlock(getCurLocalName("while.body"));

  // Store the blocks to use for break and continue.
  BreakContinueStack.push_back(CGStmt::BreakContinue(ExitBlock, LoopHeader));

  // Evaluate the conditional in the while header.
  // The evaluation of the controlling expression taks place before each
  // execution of the loop body.
  std::shared_ptr<Value> CondVal = whilestmt->getCondition()->Accept(this);

  // while(true) is common, avoid extra blocks. Be sure to correctly handle
  // break/continue though.
  bool EmitBoolCondBranch = true;
  if (std::shared_ptr<ConstantBool> CB =
          std::dynamic_pointer_cast<ConstantBool>(CondVal)) {
    if (CB->getVal()) {
      EmitBoolCondBranch = false;
    }
    // To Do: if CB->getVal() == false, optimize
  }

  // As long as the conditon is true, go to the loop body.
  if (EmitBoolCondBranch) {
    auto ret = CreateCondBr(CondVal, LoopBody, ExitBlock);
  }

  // Emit the loop body.
  EmitBlock(LoopBody);
  whilestmt->getLoopBody()->Accept(this);

  EmitBrach(LoopHeader);

  // Emit the exit block.
  EmitBlock(ExitBlock, true);

  // The LoopHeader typically is just a branch (when we EmitBlock(LoopBody),
  // will generate a unconditional branch to LoopBody.) if we skipped emitting a
  // branch, try to erase it.
  if (!EmitBoolCondBranch) {
    SimplifyForwardingBlocks(LoopHeader);
  }
}

/// If the given basic block is only a branch to another basic block, simplify
/// it.
void ModuleBuilder::SimplifyForwardingBlocks(std::shared_ptr<BasicBlock> BB) {
  if (BB->getInstList().size() != 1) {
    return;
  }
  std::shared_ptr<BranchInst> BI =
      std::dynamic_pointer_cast<BranchInst>(BB->getTerminator());
  if (!BI || !BI->isUncoditional())
    return;
  BB->replaceAllUsesWith(BI->getSuccessor(0));
  BB->removeFromParent();
}

std::shared_ptr<Value> ModuleBuilder::visit(const WhileStatement *WS) {
  EmitWhileStmt(WS);
  return nullptr;
}

/// \brief Dispatched the task to the children.
std::shared_ptr<Value> ModuleBuilder::visit(const CompoundStmt *comstmt) {
  // (1) Switch the scope.
  auto symTab = CurScope->getSymbolTable();
  auto num = symTab.size();
  for (unsigned i = 0; i < num; i++) {
    if (std::shared_ptr<ScopeSymbol> scope =
            std::dynamic_pointer_cast<ScopeSymbol>(symTab[i])) {
      if (scope->isVisitedForIRGen())
        continue;
      CurScope = scope->getScope();
      break;
    }
    continue;
  }

  // (2) Generated the code for children.
  std::size_t size = comstmt->getSize();
  for (unsigned i = 0; i < size; ++i) {
    auto ret = (*comstmt)[i]->Accept(this);
  }

  // (3) Switch the scope back.
  if (num != 0)
    CurScope = CurScope->getParent();

  return nullptr;
}

std::shared_ptr<Value> ModuleBuilder::visit(const ReturnStatement *retstmt) {
  return EmitReturnStmt(retstmt);
}

void ModuleBuilder::EmitIfStmt(const IfStatement *ifstmt) {
  // If the condition constant folds and can be elided, try to avoid emitting
  // the condition and the dead arm of the if/else
  bool CondConstant;

  /// If the condition expr can be evaluated, true or false.
  if (evaluator.EvaluateAsBooleanCondition(ifstmt->getCondition(),
                                           CondConstant)) {
    StmtASTPtr Executed = ifstmt->getThen();
    StmtASTPtr Skipped = ifstmt->getElse();
    /// Condition expression can be evaluated to the false value.
    if (!CondConstant)
      std::swap(Executed, Skipped);

    /// C/C++ has goto statement, so there is one situation that we can't elide
    /// the specified block. e.g	    if (10 != 10)
    ///	        {                    -----> Evaluate the condition expression to
    /// be false, dead 'then'. 	    RET:	return num;      -----> The
    /// 'RET' label means that there is possible that 'return num' can be
    /// execeted.
    ///	        }
    ///	        else
    ///	        {
    ///	            ...
    ///	        }
    /// But moses have no goto statements, so we don't need to worry.
    if (Executed)
      Executed->Accept(this);
    return;
  }

  // othewise, the condition did not fold, or we couldn't elide it. Just emit
  // the condition branch.

  // Create blocks for then and else cases. Insert the 'then' block at the end
  // of the function.
  auto ThenBlock = BasicBlock::Create(getCurLocalName("if.then"), nullptr);
  auto ContBlock = BasicBlock::Create(getCurLocalName("if.end"), nullptr);
  auto ElseBlock = ContBlock;
  if (ifstmt->getElse())
    ElseBlock = BasicBlock::Create(getCurLocalName("if.else"), nullptr);

  // Once the blocks are created, we can emit the conditional branch that choose
  // between them.
  EmitBranchOnBoolExpr(ifstmt->getCondition(), ThenBlock, ElseBlock);

  // Emit the 'then' code.
  EmitBlock(ThenBlock);
  ifstmt->getThen().get()->Accept(this);

  EmitBrach(ContBlock);
  // Emit the 'else' code if present.
  if (auto Else = ifstmt->getElse()) {
    EmitBlock(ElseBlock);

    Else.get()->Accept(this);

    EmitBrach(ContBlock);
  }

  // Emit the continuation block for code after the if.
  EmitBlock(ContBlock, true);
}

/// \ brief EmitReturnStmt - Generate code for ReturnStatement.
/// Note:	var num : int;
///			func print() -> void
///			{
///				num = 10;
///			}
///			func add() -> void
///			{
///				return print();   ----> This is allowed.
///				~~~~~~~~~~~~~~
///			}
///
std::shared_ptr<Value>
ModuleBuilder::EmitReturnStmt(const ReturnStatement *RS) {
  // Emit the sub-expression, even if unused, to evaluate the side effects.
  const Expr *SubE = RS->getSubExpr().get();

  if (!CurFunc->ReturnValue) {
    // Make sure not to return anything, but evaluate the expression for
    // side effects.
    if (SubE)
      SubE->Accept(this);
  } else if (SubE == nullptr) {
    // Do nothing(return value is left uninitialized).
  } else if (SubE->getType()->getKind() == TypeKind::USERDEFIED ||
             SubE->getType()->getKind() == TypeKind::ANONYMOUS) {
    EmitAggExpr(SubE, CurFunc->ReturnValue);
  } else {
    std::shared_ptr<Value> V = SubE->Accept(this);
    auto ret = CreateStore(V, CurFunc->ReturnValue);
  }
  EmitBrach(CurFunc->ReturnBlock);
  return nullptr;
}

void ModuleBuilder::EmitFunctionBody(StmtASTPtr body) {
  EmitCompoundStmt(body.get());
}

std::shared_ptr<Value>
ModuleBuilder::EmitCompoundStmt(const StatementAST *stmt) {
  stmt->Accept(this);
  return nullptr;
}
