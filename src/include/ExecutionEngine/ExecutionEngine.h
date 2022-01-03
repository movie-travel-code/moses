//===---------------------------ExecutionEngine.h-------------------------===//
//
// This file defines the class ExecutionEngine.
//
//===---------------------------------------------------------------------===//
#pragma once

#include "GenericValue.h"
#include "IR/BasicBlock.h"
#include "IR/Function.h"
#include "IR/IRType.h"
#include "IR/Instruction.h"
#include "IR/User.h"
#include "IR/Value.h"
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace Execution {
using namespace IR;
using Opcode = Instruction::Opcode;

/// AllocaHolder - Object to track all of the blocks of memory allocated by
/// alloca. When the function returns, this object is "popped off" the execution
/// stack, which causes the dtor to be run, which free all the alloca'd memory.
struct AllocaHolder {
  std::vector<std::shared_ptr<char>> Allocations;

  void add(std::shared_ptr<char> Memory) { Allocations.push_back(Memory); }
};

/// ExecutionContext struct - This struct represents one stack frame currently
/// executing.
struct ExecutionContext {
  // The currently executing function.
  std::shared_ptr<Function> CurFunction;
  // The currently executing BB
  std::shared_ptr<BasicBlock> CurBB;
  // The next instruction to execute.
  std::list<std::shared_ptr<Instruction>>::iterator CurInst;
  // Values used in this invocation.
  // http://stackoverflow.com/questions/24239696/using-std-shared-ptr-as-stdmap-key
  std::map<std::shared_ptr<Value>, GenericValue> Values;

  // Holds the call that called subframes.
  std::shared_ptr<CallInst> Caller;

  // Track memory allocated by alloca.
  AllocaHolder Allocas;
  std::shared_ptr<Instruction> IssueInstruction() {
    if (CurInst != CurBB->end())
      return *CurInst++;
    return nullptr;
  }
  ExecutionContext() : CurFunction(nullptr), CurBB(nullptr) {}
};

class Interpreter {
  // The return value of the called function.
  GenericValue ExitValue;

  // The runtime stack of executing code. The top of the stack is the current
  // function record.
  std::vector<ExecutionContext> ECStack;

  // Base frame.
  ExecutionContext TopLevelFrame;

  // AtExitHandlers - List of functions to call when the program exits,
  // registered with the atexit() library function.
  // std::vector<Function*> AtExitHandlers;
  const std::list<std::shared_ptr<Value>> &Insts;
  const MosesIRContext &Ctx;

private:
  /// Methods used to execute code:
  /// Place a call on the stack.
  void callFunction(Function *F, const std::vector<GenericValue> &ArgVals);
  /// Execute Intrinsic call.
  void callIntrinsic(std::shared_ptr<CallInst> I);

  void LoadValueFromMemory(GenericValue &Dest, GenericValue SrcAddr,
                           IR::TyPtr Ty);
  void StoreValueToMemory(GenericValue V, GenericValue DestAddr, IR::TyPtr Ty);
  void PopStackAndSetReturnValue(GenericValue ReturnValue, bool isVoid);

public:
  Interpreter(const std::list<std::shared_ptr<Value>> &Insts,
              const MosesIRContext &Ctx);
  ~Interpreter() {}

  /// runAtExitHandlers - Run any functions registered by the program's calls
  /// to atexit(3).
  /// void runAtExitHandlers();

  /// create - Create an interpreter ExecutionEngine. This can never fail.
  static std::shared_ptr<Interpreter>
  create(const std::list<std::shared_ptr<Value>> &Insts,
         const MosesIRContext &Ctx);

  /// StoreValueToMemory - Stores the data in Val of type Ty at address Ptr.
  /// Ptr is the address of the memory at which to store Val, cast to
  /// GenericValue *(It is not a pointer to a GenericValue containing the
  /// address at which to store Val).
  void StoreValueToMemory(GenericValue Val, GenericValue *Ptr, IR::TyPtr Ty);

  /// run - Start execution with the specified funciton and arguments.
  GenericValue runFunction(Function *F,
                           const std::vector<GenericValue> &ArgValues);

  // Execute one instruction.
  void executeInstruction();
  /// Execute instructions until nothing left to do.
  void run();

  void visitReturnInst(std::shared_ptr<ReturnInst> I);
  void visitBranchInst(std::shared_ptr<BranchInst> I);

  void visitBinaryOperator(std::shared_ptr<BinaryOperator> I);
  void visitCmpInst(std::shared_ptr<CmpInst> I);
  void visitAllocaInst(std::shared_ptr<AllocaInst> I);
  void visitLoadInst(std::shared_ptr<LoadInst> I);
  void visitStoreInst(std::shared_ptr<StoreInst> I);
  void visitGEPInst(std::shared_ptr<GetElementPtrInst> I);

  void visitCallInst(std::shared_ptr<CallInst> I);

  void visitShl(std::shared_ptr<BinaryOperator> I);
  void visitShr(std::shared_ptr<BinaryOperator> I);

  void visit(std::shared_ptr<Instruction> I);

  void exitCalled(GenericValue GV);

  // SwitchToNewBasicBlock - Start execution in a new basic block and run any
  // PHI nodes in the top of the block. This is used for intraprocedural
  // control flow.
  void SwitchToNewBasicBlock(std::shared_ptr<BasicBlock> New,
                             ExecutionContext &SF);

  GenericValue getOperandValue(std::shared_ptr<Value> V, ExecutionContext &SF);

  GenericValue getConstantValue(ConstantPtr C);

  GenericValue getLocalAndGlobalGV(std::shared_ptr<Value> V);

  void *getPointerToFunction(Function *F) { return (void *)F; };

  /// SetGenericValue - Update the GenericValue of specified V on the SF.
  void SetGenericValue(std::shared_ptr<Value> V, GenericValue GV,
                       ExecutionContext &SF);

  /// callFunction - Switch the context info and create a new stack frame.
  void callFunction(std::shared_ptr<Function> Function,
                    std::vector<GenericValue> ArgVals);
};
} // namespace Execution
