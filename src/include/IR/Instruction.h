//===-------------------------------Instruction.h-------------------------===//
//
// This file contains the declaration of the Instruction class, which is the
// base class for all of the moses ir instructions.
//
//===---------------------------------------------------------------------===//
#pragma once
#include "BasicBlock.h"
#include "ConstantAndGlobal.h"
#include "User.h"
#include <list>
#include <memory>
#include <string>
#include <vector>

namespace IR {
class MosesIRContext;
class Instruction : public User {
public:
  enum class Opcode {
    Ret, // ReturnInst
    Br,  // BranchInst

    Add, // BinaryOperator Integer Add
    Sub, // BinaryOperator Integer Sub
    Mul, // BinaryOperator Integer Mul
    Div, // BinaryOperator Integer Div
    Rem, // BinaryOperator Integer remain
    Shl, // BinaryOperator Integer shift left
    Shr, // BinaryOperator Integer shift right
    And, // BinaryOperator Bool And
    Or,  // BinaryOperator Bool Or
    Xor, // implements ! operator,
         // For example, '!flag' flag xor true

    Alloca,        // Stack management
    Load,          // Memory manipulation instrs load
    Store,         // Memory mainpulation instrs store
    GetElementPtr, // Struct Type's intrs

    Trunc, // anonymous type cast to struct types
    Cmp,   // Integer comparison instruciton
    PHI,   // PHI node instruction
    Call   // Call a function
  };

protected:
  std::shared_ptr<BasicBlock> Parent;
  Opcode op;
  void setParent(std::shared_ptr<BasicBlock> P);

public:
  std::shared_ptr<BasicBlock> getParent() const { return Parent; }
  // std::list<std::shared_ptr<Instruction>> getIterator() { auto BB =
  // this->getParent();	}

  /// Return the function this instruction belongs to.
  /// Note: it is undefined behavior to call this on an instruction not
  /// currently inserted into a function.
  std::shared_ptr<Function> getFunction() const;

  /// Insert an unlinked instruction into a basic block immediately after the
  /// specified intruction.
  void insertBefore(std::shared_ptr<Instruction> InsertPos);

  /// Insert an unlinked instruction into a basic block immediately after the
  /// specified instruction.
  void insertAfter(std::shared_ptr<Instruction> InsertPos);

  /// Unlink this instruction from its current basic block and insert it into
  /// the basic block that MovePos lives in, right before MovePos.
  void moveBefore(std::shared_ptr<Instruction> MovePos);

  //===---------------------------------------------------------===//
  // Subclass classification.
  //===---------------------------------------------------------===//
  /// Returns a member of one of the enums like Instruction::Add.
  Opcode getOpcode() const { return op; }
  bool isTerminator() const { return op == Opcode::Ret || op == Opcode::Br; }
  bool isBinaryOp() const { return op >= Opcode::Add && op <= Opcode::Or; }
  bool isShift() { return op == Opcode::Shl || op == Opcode::Shr; }
  bool isCast() const { return op == Opcode::Trunc; }

  //===---------------------------------------------------------===//
  // Predicates and helper methods.
  //===---------------------------------------------------------===//

  // Return true if the intruction is associative:
  //
  // Associative operators satisfy: x op (y op z) == (x op y) op z
  //
  // In moses IR, the Add, Mul, Mul, and Xor operators are associative.
  bool isAssociative() const {
    return op == Opcode::Add || op == Opcode::Mul || op == Opcode::Xor;
  }

  // Return true if the instruction is commutative:
  // Commutative operators satisfy: (x op y) == (y op x)
  bool isCommutative() const {
    /*switch (op)
                    {
                    case IR::Instruction::Opcode::Add:
                    case IR::Instruction::Opcode::Mul:
                    case IR::Instruction::Opcode::Or:
                    case IR::Instruction::Opcode::Xor:
                            return true;
                    default:
                            return false;
                    }*/
    return true;
  }

  // Return true if this instruction may modify memory.
  bool mayWriteToMemory() const;

  // Return true if this memory may read memory.
  bool mayReadFromMemory() const;

  bool mayReadOrWriteMemory() const {
    return mayReadFromMemory() || mayWriteToMemory();
  }

  // Return true if the instruciton may have side effects.
  // Note that this doest not consider malloc and alloca to have side
  // effects because the newly allocated memory is completely invisible
  // to instructions which don't use the returned value.
  bool mayHaveSideEffects() const { return mayWriteToMemory(); }

public:
  Instruction(TyPtr Ty, Opcode op, std::shared_ptr<BasicBlock> parent,
              const std::string &Name = "");
};

//===-------------------------------------------------------------===//
//						UnaryInstruction Class
//===-------------------------------------------------------------===//
class UnaryOperator : public Instruction {
protected:
  UnaryOperator(TyPtr Ty, Opcode op, std::shared_ptr<Value> V,
                std::shared_ptr<BasicBlock> parent,
                [[maybe_unused]] std::shared_ptr<Instruction> IB = nullptr)
      : Instruction(Ty, op, parent) {
    // Operands.resize(1);
    Operands.push_back(Use(V, this));
  }

public:
  static bool classof(std::shared_ptr<Instruction> I) {
    return I->getOpcode() == Instruction::Opcode::Alloca ||
           I->getOpcode() == Instruction::Opcode::Load;
  }
};

//===-------------------------------------------------------------===//
//						TerminatorInst Class
//===-------------------------------------------------------------===//

/// Subclasses of this class are able to terminate a basic block. Thus,
/// these are all the flow control type of operations.
class TerminatorInst : public Instruction {
protected:
  TerminatorInst(
      TyPtr Ty, Instruction::Opcode op, std::shared_ptr<BasicBlock> parent,
      [[maybe_unused]] std::shared_ptr<BasicBlock> InsertAtEnd = nullptr)
      : Instruction(Ty, op, parent) {}
  // Out of line virtual method, so the vtable, etc has a home.
  ~TerminatorInst() override;
  //===---------------------------------------------------------------------===//
  //
  //===---------------------------------------------------------------------===//

public:
  /// Return the number of successors that this terminator hsa.
  virtual unsigned getNumSuccessors() const = 0;

  /// Return the specified successor.
  virtual std::shared_ptr<BasicBlock> getSuccessor(unsigned idx) const = 0;

  /// Update the specified successor to point at the provided block.
  virtual void setSuccessor(unsigned idx, std::shared_ptr<BasicBlock> B) = 0;

  // Methods for support type inquiry through isa, cast, and dyn_cast:
  static bool classof(std::shared_ptr<Instruction> I) {
    return I->isTerminator();
  }
};

//===-------------------------------------------------------------===//
//						BinaryOperator Class
//===-------------------------------------------------------------===//
class BinaryOperator final : public Instruction {
public:
  BinaryOperator(Opcode op, std::shared_ptr<Value> S1,
                 std::shared_ptr<Value> S2, TyPtr Ty,
                 std::shared_ptr<BasicBlock> parent,
                 const std::string &Name = "");
  ~BinaryOperator() override;
  // BinaryOperator(Opcode op, std::shared_ptr<Value> S1, std::shared_ptr<Value>
  // S2, TyPtr Ty);

  /// Construct a binary instruction, given the opcode and the two operands.
  /// Optionally (if InstBefore is specified) insert the instruction into a
  /// BasicBlock right before the specified instruction.
  // static std::shared_ptr<BinaryOperator> Create(Opcode op,
  // std::shared_ptr<Value> S1, std::shared_ptr<Value> S2,
  // std::shared_ptr<Instruction> InsertBefore = nullptr);

  /// Construct a binary instruction, given the opcode and the two operands.
  /// Also automatically insert this instruction to the end of the BasicBlock
  /// specified.
  static std::shared_ptr<BinaryOperator>
  Create(Opcode op, std::shared_ptr<Value> S1, std::shared_ptr<Value> S2,
         std::shared_ptr<BasicBlock> parent,
         std::shared_ptr<BasicBlock> InsertAtEnd = nullptr);

  static std::shared_ptr<BinaryOperator>
  Create(Opcode op, std::shared_ptr<Value> S1, std::shared_ptr<Value> S2,
         std::shared_ptr<BasicBlock> parent,
         std::shared_ptr<Instruction> InsertBefore);

  static std::shared_ptr<BinaryOperator>
  CreateNeg(MosesIRContext &Ctx, std::shared_ptr<Value> Operand,
            std::shared_ptr<BasicBlock> parent,
            std::shared_ptr<Instruction> InsertBefore = nullptr);

  static std::shared_ptr<BinaryOperator>
  CreateNeg(MosesIRContext &Ctx, std::shared_ptr<Value> Operand,
            std::shared_ptr<BasicBlock> parent,
            std::shared_ptr<BasicBlock> InsertAtEnd);

  // Shit code!
  static std::shared_ptr<BinaryOperator>
  CreateNot(MosesIRContext &Ctx, std::shared_ptr<Value> Operand,
            std::shared_ptr<BasicBlock> parent,
            std::shared_ptr<Instruction> InsertBefore = nullptr);

  // Shit code!
  static std::shared_ptr<BinaryOperator>
  CreateNot(MosesIRContext &Ctx, std::shared_ptr<Value> Operand,
            std::shared_ptr<BasicBlock> parent,
            std::shared_ptr<BasicBlock> InsertAtEnd);
  static bool isNeg(std::shared_ptr<Value> V);
  static bool isNot(std::shared_ptr<Value> V);

  // Methods for support type inquiry through isa, cast, and dyn_cast:
  static bool classof(std::shared_ptr<Instruction> I) {
    return I->isBinaryOp();
  }

  /// \brief Print the BinaryOperator.
  void Print(std::ostringstream &out) override;
};

//===-------------------------------------------------------------===//
//						TruncInst Class
//===-------------------------------------------------------------===//
// class TruncInst : public UnaryOperator
//{
//	// TrancInst
//};

//===-------------------------------------------------------------===//
//						CmpInst Class
//===-------------------------------------------------------------===//

class CmpInst final : public Instruction {
public:
  /// This enumeration lists the possible predicates for CmpInst.
  enum Predicate {
    // Opcode
    CMP_EQ,
    CMP_NE,
    CMP_GT,
    CMP_GE,
    CMP_LT,
    CMP_LE
  };

private:
  CmpInst() = delete;
  Predicate predicate;

public:
  CmpInst(MosesIRContext &Ctx, std::shared_ptr<Instruction> InsertBefore,
          Predicate pred, std::shared_ptr<Value> LHS,
          std::shared_ptr<Value> RHS, std::shared_ptr<BasicBlock> parent,
          const std::string &Name = "");
  // Out-of-line method.
  ~CmpInst() override;

  /// Construct a compare instruction, given the opcode, the predicaee and
  /// the two operands. Optionally (if InstBefore is specified) insert the
  /// instruction into a BasicBlock right before the specified instruction
  /// @brief Create a CmpInst
  static std::shared_ptr<CmpInst>
  Create(MosesIRContext &Ctx, Predicate predicate, std::shared_ptr<Value> S1,
         std::shared_ptr<Value> S2, std::shared_ptr<BasicBlock> parent,
         const std::string &name = "",
         std::shared_ptr<Instruction> InsertBefore = nullptr);

  static std::shared_ptr<CmpInst>
  Create(MosesIRContext &Ctx, Predicate predicate, std::shared_ptr<Value> S1,
         std::shared_ptr<Value> S2, std::shared_ptr<BasicBlock> parent,
         const std::string &name, std::shared_ptr<BasicBlock> InsertAtEnd);

  Predicate getPredicate() const { return predicate; }
  void setPredicate(Predicate P) { predicate = P; }
  bool isEquality() const { return predicate == Predicate::CMP_EQ; }

  /// @brief Methodds for support type inquiry through isa, cast, and dyn_cast
  static bool classof(std::shared_ptr<Instruction> I) {
    return I->getOpcode() == Instruction::Opcode::Cmp;
  }

  /// \brief Print the CmpInst.
  void Print(std::ostringstream &out) override;
};

//===-------------------------------------------------------------===//
//						AllocaInst Class
//===-------------------------------------------------------------===//
/// AllocaInst - an instruction to allocate memory on the stack.
class AllocaInst final : public UnaryOperator {
  TyPtr AllocatedType;

public:
  // AllocaInst(TyPtr Ty,/* std::shared_ptr<Value> ArraySize, */std::string Name
  // = "", std::shared_ptr<Instruction> InsertBefore = nullptr);
  explicit AllocaInst(TyPtr Ty, std::shared_ptr<BasicBlock> parent,
                      const std::string &Name = "",
                      std::shared_ptr<BasicBlock> InsertAtEnd = nullptr);
  // Out-of-line method.
  ~AllocaInst() override;
  static std::shared_ptr<AllocaInst> Create(TyPtr Ty,
                                            std::shared_ptr<BasicBlock> parent,
                                            const std::string &Name = "");

  /// getAllocatedType - Return the type that is being allocated by the
  /// instruction.
  TyPtr getAllocatedType() const { return AllocatedType; }

  /// \brief Print the AllocaInst.
  void Print(std::ostringstream &out) override;
};

//===-------------------------------------------------------------===//
//						GetElementPtrInst Class
//===-------------------------------------------------------------===//
// GetElementPtrInst - an instruction for type-safe pointer arithmetic
// to access elements of arrays and structs.
class GetElementPtrInst final : public Instruction {
  GetElementPtrInst(const GetElementPtrInst &GEPI) = delete;
  void init(std::shared_ptr<Value> Ptr,
            std::vector<std::shared_ptr<Value>> IdxList);
  void init(std::shared_ptr<Value> Ptr, std::shared_ptr<Value> Idx0,
            std::shared_ptr<Value> Idx1);

public:
  /// Constructors - Create a getelementptr instruction with a base pointer and
  /// an list of indices.
  GetElementPtrInst(TyPtr PointeeType, std::shared_ptr<Value> Ptr,
                    std::vector<std::shared_ptr<Value>> IdxList,
                    std::shared_ptr<BasicBlock> parent,
                    const std::string &Name = "",
                    std::shared_ptr<Instruction> InsertBefore = nullptr);

  /// Constructors - This constructions is convenience method because two
  /// index getelementptr instructions are so common.
  GetElementPtrInst(TyPtr PointeeType, std::shared_ptr<Value> Ptr,
                    std::shared_ptr<Value> Idx0, std::shared_ptr<Value> Idx1,
                    std::shared_ptr<BasicBlock> parent,
                    const std::string &Name = "");

public:
  static std::shared_ptr<GetElementPtrInst>
  Create(TyPtr PointeeType, std::shared_ptr<Value> Ptr,
         std::vector<std::shared_ptr<Value>> IdxList,
         std::shared_ptr<BasicBlock> parent, const std::string &Name = "",
         std::shared_ptr<Instruction> InsertBefore = nullptr);
  static std::shared_ptr<GetElementPtrInst>
  Create(TyPtr PointeeType, std::shared_ptr<Value> Ptr,
         std::shared_ptr<Value> Idx0, std::shared_ptr<Value> Idx1,
         std::shared_ptr<BasicBlock> parent, const std::string &Name = "",
         std::shared_ptr<Instruction> InsertBefore = nullptr);

  std::shared_ptr<Value> getPointerOperand();
  static unsigned getPointerOperandIndex() { return 0; }
  unsigned getNumIndices() const { return getNumOperands() - 1; }

  /// \brief Print the GetElementPtrInst.
  void Print(std::ostringstream &out);
};

//===-------------------------------------------------------------===//
//						CallInst
//===-------------------------------------------------------------===//
// ClassInst - This class represents a function call, abstracting a
// target mechine's calling convention. This class uses low bit of
// the SubClassData field to indicate whether or not this is a tail
// call. The rest of the bits hold the calling convention of the call.
class CallInst final : public Instruction {
private:
  std::shared_ptr<FunctionType> FTy;
  bool IsIntrisicCall;

public:
  CallInst(std::shared_ptr<FunctionType> FTy, std::shared_ptr<Value> Func,
           std::vector<std::shared_ptr<Value>> Args,
           std::shared_ptr<BasicBlock> parent, const std::string &Name = "",
           std::shared_ptr<BasicBlock> InsertAtEnd = nullptr);
  CallInst(std::shared_ptr<Intrinsic> Intr,
           std::vector<std::shared_ptr<Value>> Args,
           std::shared_ptr<BasicBlock> parent, const std::string &Name = "",
           std::shared_ptr<Instruction> InsertBefore = nullptr);

  static std::shared_ptr<CallInst>
  Create(std::shared_ptr<Value> Func, std::vector<std::shared_ptr<Value>> Args,
         std::shared_ptr<BasicBlock> parent, const std::string &Name = "",
         std::shared_ptr<Instruction> InsertBefore = nullptr);

  static std::shared_ptr<CallInst>
  Create(std::shared_ptr<Intrinsic> Intr,
         std::vector<std::shared_ptr<Value>> Args,
         std::shared_ptr<BasicBlock> parent, const std::string &Name = "",
         std::shared_ptr<Instruction> InsertBefore = nullptr);

  /*static std::shared_ptr<CallInst> Create(std::shared_ptr<FunctionType> *Ty,
     std::shared_ptr<Value> Func, std::list<std::shared_ptr<Value>> Args,
     std::string NameStr, std::shared_ptr<Instruction> InsertBefore =
     nullptr);*/
  // Out-of-line method.
  ~CallInst() override;

  std::shared_ptr<FunctionType> getFunctionType() const { return FTy; }

  enum class TailCallKind {
    TCK_None = 0,
    TCK_Tail = 1,
    TCK_MustTail = 2,
    TCK_NoTail = 3
  };

  bool isIntrinsicCall() const { return IsIntrisicCall; }

  // To Do:
  TailCallKind getTailCallKind() const { return TailCallKind::TCK_None; }
  bool isTailCall() const { return true; }
  bool isMustTailCall() const { return true; }
  bool isNoTailCall() const { return true; }
  void setTailCall([[maybe_unused]] bool isTC = true) {}
  void setTailCallKind([[maybe_unused]] TailCallKind TCK) {}

  /// getNumArgOperands - Return the number of call arguments.
  unsigned getNumArgOperands() const { return getNumOperands() - 1; }
  std::shared_ptr<Value> getArgOperand(unsigned i) const;
  void setArgOperand(unsigned i, std::shared_ptr<Value> v);

  std::shared_ptr<Value> getCalledFunction() const { return Operands[0].get(); }

  /// getCalledValue - Get a pointer to the function that is invoked by this
  /// instruction.
  std::shared_ptr<Value> getCalledValue() const { return getCalledFunction(); }
  void setCalledFunction(std::shared_ptr<Value> Fn) { Operands[0] = Fn; }
  void setCalledFunction(std::shared_ptr<FunctionType> Ty,
                         std::shared_ptr<Value> Fn) {
    FTy = Ty;
    Operands[0] = Fn;
  }

  // Methods for support type inquiry through isa, cast, and dyn_cast:
  static bool classof(std::shared_ptr<Instruction> I) {
    return I->getOpcode() == Instruction::Opcode::Call;
  }

  /// \brief Print the CallInst.
  void Print(std::ostringstream &out) override;
};

//===---------------------------------------------------------------===//
//						ExtractValueInst Class
//===---------------------------------------------------------------===//
// ExtractValueInst - This instruction extracts a struct member or array
// element value from an aggregate value.
// <result> = extractvalue <aggregate type> <val>, <idx>{, <idx>}*
// Example:  <result> = extractvalue {i32, float} %agg, 0
class ExtractValueInst final : public UnaryOperator {
private:
  TyPtr AggregateType;

  ExtractValueInst(const ExtractValueInst &EVI);
  void init(std::vector<unsigned> Idxs, std::string NameStr);

  /// Constructors - Create a extractvalue instruction with a base aggregate
  /// value and a list of indices. The first ctor can optionally insert before
  /// an existing instruction, the second appends the new instruction to the
  // specified BasicBlock.
  ExtractValueInst(std::shared_ptr<Value> Agg, std::vector<unsigned> Idxs,
                   std::string NameStr,
                   std::shared_ptr<Instruction> InsertBefore = nullptr);
  ExtractValueInst(std::shared_ptr<Value> Agg, std::vector<unsigned> Idxs,
                   std::string NameStr,
                   std::shared_ptr<BasicBlock> InsertAtEnd);
  // allocate space for exactly one operand
  // void *operator new(size_t s){ return User::operator new(s, 1); }
public:
  // Out-of-line method.
  ~ExtractValueInst() override;

  static std::shared_ptr<ExtractValueInst>
  Create([[maybe_unused]] std::shared_ptr<Value> Agg,
         [[maybe_unused]] std::vector<unsigned> Idxs,
         [[maybe_unused]] const std::string &NameStr = "",
         [[maybe_unused]] std::shared_ptr<Instruction> InsertBefore = nullptr) {
    // return new ExtractValueInst(Agg, Idxs, NameStr, InsertBefore);
    return nullptr;
  }

  static std::shared_ptr<ExtractValueInst>
  Create([[maybe_unused]] std::shared_ptr<Value> Agg,
         [[maybe_unused]] std::vector<unsigned> Idxs,
         [[maybe_unused]] const std::string &NameStr,
         [[maybe_unused]] std::shared_ptr<BasicBlock> InsertAtEnd = nullptr) {
    // return new ExtractValueInst(Agg, Idxs, NameStr, InsertAtEnd);
    return nullptr;
  }

  /// \brief Print the ExtractValueInst.
  void Print(std::ostringstream &out) override;
};

//===---------------------------------------------------------------===//
//						PHINode Class
//===---------------------------------------------------------------===//

// PHINode - The PHINode class is used to represent the magical mystical
// (���ص�)PHI node, that can not exist in nature, but can be synthesized
// in a computer scientist's overactive imagination.
class PHINode final : public Instruction {
private:
  // void *operator new(size_t, unsigned) = delete;
  /// ReservedSpace - The number of operands actually allocated. NumOperands
  /// is the number actually in use.
  PHINode(const PHINode &PN);

  // allocate space for exactly zero operands
  /*void *operator new(size_t s)
                  {
                          return User::operator new(s);
                  }*/

  explicit PHINode(
      TyPtr Ty, [[maybe_unused]] unsigned NumReservedValues,
      std::shared_ptr<BasicBlock> parent,
      [[maybe_unused]] const std::string &NameStr = "",
      [[maybe_unused]] std::shared_ptr<Instruction> InsertBefore = nullptr)
      : Instruction(Ty, Instruction::Opcode::PHI, parent) {}

protected:
  // allocHungoffUses - this is moew complicated than the generic
  // User::allocHungoffUses, because we have to allocate Uses for the incoming
  // values and pointers to the incoming blocks, all in one allocation.
  void allocHungoffUses([[maybe_unused]] unsigned N) {}

public:
  // Constructors - NumReservedValues is a hint for the number of incoming
  // edges that this phi node will have(use 0 if you really have no idea).
  static std::shared_ptr<PHINode>
  Create([[maybe_unused]] TyPtr Ty, [[maybe_unused]] unsigned NumReservedValues,
         [[maybe_unused]] const std::string &NameStr = "",
         [[maybe_unused]] std::shared_ptr<Instruction> InsertBefore = nullptr) {
    return nullptr;
  }

  static std::shared_ptr<PHINode>
  Create([[maybe_unused]] TyPtr Ty, [[maybe_unused]] unsigned NumReservedValues,
         [[maybe_unused]] const std::string &NameStr,
         [[maybe_unused]] std::shared_ptr<BasicBlock> InsertAtEnd) {
    return nullptr;
  }
  // Block iterator interface. This provides access to the list of incoming
  // basic blocks, which parallels the list of incoming values.

  // getNumIncomingValues - Return the number of incoming edges
  unsigned getNumIncomingValues() const { return getNumOperands(); }

  // getIncomingValue - Return incoming value number x
  std::shared_ptr<Value> getIncomingValue([[maybe_unused]] unsigned i) const {
    return nullptr;
  }
  void setIncomingValue([[maybe_unused]] unsigned i,
                        [[maybe_unused]] std::shared_ptr<Value> V) {}
  /// getIncomingBlock - Return incoming basic block number @p i.
  std::shared_ptr<BasicBlock>
  getIncomingBlock([[maybe_unused]] unsigned i) const {
    return nullptr;
  }

  /// getIncomingBlock - Return incoming basic block corresponding
  // to an operand of the PHI.
  std::shared_ptr<BasicBlock>
  getIncomingBlock([[maybe_unused]] const Use &U) const {
    return nullptr;
  }
  void setIncomingBlock([[maybe_unused]] unsigned i,
                        [[maybe_unused]] std::shared_ptr<BasicBlock> BB) {}
  /// addIncoming - Add an incoming value to the end of the PHI list.
  void addIncoming([[maybe_unused]] std::shared_ptr<Value> V,
                   [[maybe_unused]] std::shared_ptr<BasicBlock> BB) {}

  /// removeIncomingValue - Remove an incoming value. This is useful if a
  /// predecessor basic block is deleted. The value removed is returned.
  std::shared_ptr<Value> removeIncomingValue(unsigned Idx,
                                             bool DeletePHIIfEmpty = true);

  // getBasicBlockIdx - Return the first index of the specified basic block
  // in the value list for this PHI. Returns -1 if no instance.
  int getBasicBlockIndex(
      [[maybe_unused]] std::shared_ptr<BasicBlock> BB) const {
    return 0;
  }

  std::shared_ptr<Value> getIncomingValueForBlock(
      [[maybe_unused]] std::shared_ptr<BasicBlock> BB) const {
    return nullptr;
  }

  /// hasConstantValue - If the specified PHI node always merges together the
  /// same value, return the value, otherwise return null.
  /// ���ĳ��Incoming blockʼ���ǲ��ɴ��
  std::shared_ptr<Value> hasConstantValue() const;

  /// Methods for support type inquiry through isa, cast and dyn_cast:
  static inline bool classof(std::shared_ptr<Instruction> I) {
    return I->getOpcode() == Instruction::Opcode::PHI;
  }

  /// \brief Print the PHINode.
  void Print(std::ostringstream &out);
};

//===----------------------------------------------------------------===//
//						ReturnInst Class
//===----------------------------------------------------------------===//
// ReturnInst - Return a value (possibly void), from a function. Execution
// does not continue in this  function any longer.
class ReturnInst final : public TerminatorInst {
  ReturnInst(const ReturnInst &RI) = delete;

public:
  // Note: If the Value* passed is of type void then the constructor behave as
  // if it was passed NULL.
  // ReturnInst(std::shared_ptr<Value> retVal = nullptr,
  // std::shared_ptr<Instruction> InsertBefore = nullptr);
  ReturnInst(std::shared_ptr<BasicBlock> parent,
             std::shared_ptr<Value> retVal = nullptr,
             std::shared_ptr<BasicBlock> InsertAtEnd = nullptr);
  ~ReturnInst() override;
  static std::shared_ptr<ReturnInst>
  Create(std::shared_ptr<BasicBlock> parent,
         std::shared_ptr<Value> retVal = nullptr,
         std::shared_ptr<BasicBlock> InsertAtEnd = nullptr);

  std::shared_ptr<Value> getReturnValue() const {
    return Operands.empty() ? nullptr : Operands[0].get();
  }

  std::shared_ptr<BasicBlock>
  getSuccessor([[maybe_unused]] unsigned index) const override {
    assert(0 && "ReturnInst has no successor!");
    return 0;
  }
  unsigned getNumSuccessors() const override {
    assert(0 && "ReturnInst has no successor!");
    return 0;
  }
  void setSuccessor(unsigned idx, std::shared_ptr<BasicBlock> B) override;

  // Method for support type inquiry through isa, cast and dyn_cast:
  static bool classof(std::shared_ptr<Instruction> I) {
    return I->getOpcode() == Instruction::Opcode::Ret;
  }

  /// \brief Print the ReturnInst.
  void Print(std::ostringstream &out) override;
};

//===----------------------------------------------------------------===//
//						BranchInst Class
//===----------------------------------------------------------------===//

// BranchInst - Conditional or Unconditional Branch instruction.
class BranchInst : public TerminatorInst {
  BranchInst(const BranchInst &BI) = delete;

  // BranchInst(BB *B)							- 'br B'
  // BranchInst(BB* T, BB *F, Value *C)			- 'br C, T, F'
  // BranchInst(BB *B, Inst *I)					- 'br B'
  // insert before
  // I
  // BranchInst(BB *T, BB *F, Value *C, Inst *I)	- 'br C, T, F'	insert
  // before I
  // BranchINst(BB *B, BB *I)						- 'br B'
  // insert at end BranchhInst(BB *T, BB *F, Value *C, BB *I)	- 'br C, T, F'
  // insert at end
public:
  BranchInst(MosesIRContext &Ctx, std::shared_ptr<BasicBlock> IfTrue,
             std::shared_ptr<BasicBlock> parent,
             std::shared_ptr<BasicBlock> InsertAtEnd = nullptr);
  BranchInst(MosesIRContext &Ctx, std::shared_ptr<BasicBlock> IfTrue,
             std::shared_ptr<BasicBlock> IfFalse, std::shared_ptr<Value> Cond,
             std::shared_ptr<BasicBlock> parent,
             std::shared_ptr<BasicBlock> InsertAtEnd = nullptr);
  static std::shared_ptr<BranchInst>
  Create(MosesIRContext &Ctx, std::shared_ptr<BasicBlock> IfTrue,
         std::shared_ptr<BasicBlock> parent,
         std::shared_ptr<BasicBlock> InsertAtEnd = nullptr);
  static std::shared_ptr<BranchInst>
  Create(MosesIRContext &Ctx, std::shared_ptr<BasicBlock> IfTrue,
         std::shared_ptr<BasicBlock> IfFalse, std::shared_ptr<Value> Cond,
         std::shared_ptr<BasicBlock> parent,
         std::shared_ptr<BasicBlock> InsertAtEnd = nullptr);
  ~BranchInst() override;
  bool isUncoditional() const { return getNumOperands() == 1; }
  bool isConditional() const { return getNumOperands() == 3; }
  std::shared_ptr<Value> getCondition() const {
    assert(Operands.size() > 1 &&
           "Condition branch instruction must have 3 operands!");
    return Operands[2].get();
  }
  void setCondition(std::shared_ptr<Value> V) {
    assert(Operands.size() > 1 &&
           "Condition branch instruction must have 3 operands!");
    Operands[2] = V;
  }
  unsigned getNumSuccessors() const override { return 1 + isConditional(); }
  std::shared_ptr<BasicBlock> getSuccessor(unsigned i) const override;
  void setSuccessor(unsigned idx, std::shared_ptr<BasicBlock> NewSucc) override;

  // Methods for support type inquiry through isa, cast and dyn_cast:
  static bool classof(std::shared_ptr<Instruction> I) {
    return I->getOpcode() == Instruction::Opcode::Br;
  }

  /// \brief Print the BranchInst.
  void Print(std::ostringstream &out) override;
};

//===----------------------------------------------------------------===//
// LoadInst - an instruction for reading from memory.
class LoadInst final : public UnaryOperator {
public:
  LoadInst(std::shared_ptr<Value> Ptr, std::shared_ptr<BasicBlock> parent,
           const std::string &Name = "",
           std::shared_ptr<BasicBlock> InsertAtEnd = nullptr);
  ~LoadInst() override;
  static std::shared_ptr<LoadInst> Create(std::shared_ptr<Value> Ptr,
                                          std::shared_ptr<BasicBlock> parent);

  std::shared_ptr<Value> getPointerOperand() const {
    assert(!Operands.empty() && "Load instruciton's operand may not be null");
    return Operands[0].get();
  }

  static bool classof(std::shared_ptr<LoadInst>) { return true; }
  static bool classof(std::shared_ptr<Instruction> I) {
    return I->getOpcode() == Instruction::Opcode::Load;
  }

  /// \brief Print the LoadInst.
  void Print(std::ostringstream &out) override;
};

//===----------------------------------------------------------------===//
// StoreInst - an instruction for storing to memory.
class StoreInst final : public Instruction {
  void init(std::shared_ptr<Value> Val, std::shared_ptr<Value> Ptr);

public:
  // StoreInst(std::shared_ptr<Value> Val, std::shared_ptr<Value> Ptr,
  // std::shared_ptr<Instruction> InsertBefore = nullptr);
  StoreInst(MosesIRContext &Ctx, std::shared_ptr<Value> Val,
            std::shared_ptr<Value> Ptr, std::shared_ptr<BasicBlock> parent,
            std::shared_ptr<BasicBlock> InsertAtEnd = nullptr);

  static std::shared_ptr<StoreInst> Create(MosesIRContext &Ctx,
                                           std::shared_ptr<Value> Val,
                                           std::shared_ptr<Value> Ptr,
                                           std::shared_ptr<BasicBlock> parent);
  static bool classof(std::shared_ptr<StoreInst>) { return true; }
  static bool classof(std::shared_ptr<Instruction> I) {
    return I->getOpcode() == Instruction::Opcode::Store;
  }

  /// \brief Print the StoreInst.
  void Print(std::ostringstream &out);
};
} // namespace IR
