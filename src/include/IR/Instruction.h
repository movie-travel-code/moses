//===-------------------------------Instruction.h-------------------------===//
//
// This file contains the declaration of the Instruction class, which is the
// base class for all of the moses ir instructions.
//
//===---------------------------------------------------------------------===//
#ifndef MOSES_IR_INSTRUCTION_H
#define MOSES_IR_INSTRUCTION_H
#include <list>
#include <vector>
#include <string>
#include <memory>
#include "BasicBlock.h"
#include "User.h"
#include "ConstantAndGlobal.h"
namespace compiler
{
	namespace IR
	{
		class MosesIRContext;
		class Instruction : public User
		{
		public:
			// LLVM的Opcode是定义在Instruction.def中的，方便扩展，如果用户欲扩展LLVM指令集
			// 修改Instruction.def即可，不需要修改源码。
			// 这里简单起见，直接在enum opcode定义在源码中
			enum class Opcode
			{
				Ret,		// ReturnInst
				Br,			// BranchInst

				Add,		// BinaryOperator Integer Add
				Sub,		// BinaryOperator Integer Sub
				Mul,		// BinaryOperator Integer Mul
				Div,		// BinaryOperator Integer Div
				Rem,		// BinaryOperator Integer remain
				Shl,		// BinaryOperator Integer shift left
				Shr,		// BinaryOperator Integer shift right
				And,		// BinaryOperator Bool And
				Or,			// BinaryOperator Bool Or
				Xor,		// implements ! operator, 
							// For example, '!flag' flag xor true

				Alloca,			// Stack management
				Load,			// Memory manipulation instrs load
				Store,			// Memory mainpulation instrs store
				GetElementPtr,	// Struct Type's intrs

				Trunc,			// anonymous type cast to struct types
				Cmp,			// Integer comparison instruciton
				PHI,			// PHI node instruction
				Call			// Call a function
			};

		protected:
			BBPtr Parent;
			Opcode op;
			void setParent(BBPtr P);
		public:
			BBPtr getParent() const { return Parent; }
			// std::list<InstPtr> getIterator() { auto BB = this->getParent();	}

			/// Return the function this instruction belongs to.
			/// Note: it is undefined behavior to call this on an instruction not
			/// currently inserted into a function.
			FuncPtr getFunction() const;

			/// Insert an unlinked instruction into a basic block immediately after the
			/// specified intruction.
			void insertBefore(InstPtr InsertPos);

			/// Insert an unlinked instruction into a basic block immediately after the
			/// specified instruction.
			void insertAfter(InstPtr InsertPos);

			/// Unlink this instruction from its current basic block and insert it into
			/// the basic block that MovePos lives in, right before MovePos.
			void moveBefore(InstPtr MovePos);

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
			bool isAssociative() const 
			{ 
				return op == Opcode::Add || op == Opcode::Mul || op == Opcode::Xor;
			}
			
			// Return true if the instruction is commutative:
			// Commutative operators satisfy: (x op y) == (y op x)			
			bool isCommutative() const
			{
				/*switch (op)
				{
				case compiler::IR::Instruction::Opcode::Add:
				case compiler::IR::Instruction::Opcode::Mul:
				case compiler::IR::Instruction::Opcode::Or:
				case compiler::IR::Instruction::Opcode::Xor:
					return true;
				default:
					return false;
				}*/
			}

			// Return true if this instruction may modify memory.
			bool mayWriteToMemory() const;

			// Return true if this memory may read memory.
			bool mayReadFromMemory() const;

			bool mayReadOrWriteMemory() const { return mayReadFromMemory() || mayWriteToMemory(); }

			// Return true if the instruciton may have side effects.
			// Note that this doest not consider malloc and alloca to have side 
			// effects because the newly allocated memory is completely invisible
			// to instructions which don't use the returned value.
			bool mayHaveSideEffects() const { return mayWriteToMemory(); }
		public:
			Instruction(TyPtr Ty, Opcode op, BBPtr parent, std::string Name = "");
		};

		//===-------------------------------------------------------------===//
		//						UnaryInstruction Class
		//===-------------------------------------------------------------===//
		class UnaryOperator : public Instruction
		{
		protected:
			UnaryOperator(TyPtr Ty, Opcode op, ValPtr V, BBPtr parent, InstPtr IB = nullptr)
				: Instruction(Ty, op, parent)
			{
				// Operands.resize(1);
				Operands.push_back(Use(V, this));
			}
		public:
			static bool classof(InstPtr I)
			{
				return I->getOpcode() == Instruction::Opcode::Alloca || 
						I->getOpcode() == Instruction::Opcode::Load;
			}
		};

		//===-------------------------------------------------------------===//
		//						TerminatorInst Class
		//===-------------------------------------------------------------===//

		/// Subclasses of this class are able to terminate a basic block. Thus,
		/// these are all the flow control type of operations.
		class TerminatorInst : public Instruction
		{
		protected:
			TerminatorInst(TyPtr Ty, Instruction::Opcode op, BBPtr parent, BBPtr InsertAtEnd = nullptr) : 
				Instruction(Ty, op, parent)
			{}
			// Out of line virtual method, so the vtable, etc has a home.
			~TerminatorInst() override;
			//===---------------------------------------------------------------------===//
			// 
			//===---------------------------------------------------------------------===//

		public:
			/// Return the number of successors that this terminator hsa.
			virtual unsigned getNumSuccessors() const = 0;

			/// Return the specified successor.
			virtual BBPtr getSuccessor(unsigned idx) const = 0;

			/// Update the specified successor to point at the provided block.
			virtual void setSuccessor(unsigned idx, BBPtr B) = 0;

			// Methods for support type inquiry through isa, cast, and dyn_cast:
			static bool classof(InstPtr I)
			{
				return I->isTerminator();
			}
		};

		//===-------------------------------------------------------------===//
		//						BinaryOperator Class
		//===-------------------------------------------------------------===//
		class BinaryOperator final : public Instruction
		{
		public:
			BinaryOperator(Opcode op, ValPtr S1, ValPtr S2, TyPtr Ty, BBPtr parent,
				std::string Name = "");
			~BinaryOperator() override;
			// BinaryOperator(Opcode op, ValPtr S1, ValPtr S2, TyPtr Ty);

			/// Construct a binary instruction, given the opcode and the two operands.
			/// Optionally (if InstBefore is specified) insert the instruction into a 
			/// BasicBlock right before the specified instruction.
			// static BOInstPtr Create(Opcode op, ValPtr S1, ValPtr S2, InstPtr InsertBefore = nullptr);

			/// Construct a binary instruction, given the opcode and the two operands.
			/// Also automatically insert this instruction to the end of the BasicBlock
			/// specified.
			static BOInstPtr Create(Opcode op, ValPtr S1, ValPtr S2, BBPtr parent, 
				BBPtr InsertAtEnd = nullptr);

			static BOInstPtr Create(Opcode op, ValPtr S1, ValPtr S2, BBPtr parent, 
				InstPtr InsertBefore);

			static BOInstPtr CreateNeg(MosesIRContext& Ctx, ValPtr Operand, BBPtr parent, 
				InstPtr InsertBefore = nullptr);

			static BOInstPtr CreateNeg(MosesIRContext& Ctx, ValPtr Operand, BBPtr parent, 
				BBPtr InsertAtEnd);

			// Shit code!
			static BOInstPtr CreateNot(MosesIRContext& Ctx, ValPtr Operand, BBPtr parent, 
				InstPtr InsertBefore = nullptr);

			// Shit code!
			static BOInstPtr CreateNot(MosesIRContext& Ctx, ValPtr Operand, BBPtr parent, BBPtr InsertAtEnd);
			static bool isNeg(ValPtr V);
			static bool isNot(ValPtr V);

			// Methods for support type inquiry through isa, cast, and dyn_cast:
			static bool classof(InstPtr I) { return I->isBinaryOp(); }

			/// \brief Print the BinaryOperator.
			void Print(std::ostringstream& out);
		};

		//===-------------------------------------------------------------===//
		//						TruncInst Class
		//===-------------------------------------------------------------===//
		// 在moses中唯一需要执行 cast 操作的就是匿名类型赋值给用户自定义类型的转换
		//class TruncInst : public UnaryOperator
		//{
		//	// TrancInst 也有operand，operand的类型必须是type类型的
		//};

		//===-------------------------------------------------------------===//
		//						CmpInst Class
		//===-------------------------------------------------------------===//

		class CmpInst final : public Instruction
		{
		public:
			/// This enumeration lists the possible predicates for CmpInst.
			enum Predicate
			{
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
			CmpInst(MosesIRContext &Ctx, InstPtr InsertBefore, Predicate pred, ValPtr LHS, 
				ValPtr RHS, BBPtr parent, std::string Name = "");
			// Out-of-line method.
			~CmpInst() override;

			/// Construct a compare instruction, given the opcode, the predicaee and
			/// the two operands. Optionally (if InstBefore is specified) insert the
			/// instruction into a BasicBlock right before the specified instruction
			/// @brief Create a CmpInst
			static CmpInstPtr Create(MosesIRContext &Ctx, Predicate predicate, ValPtr S1, 
				ValPtr S2, BBPtr parent, std::string name = "", InstPtr InsertBefore = nullptr);

			static CmpInstPtr Create(MosesIRContext &Ctx, Predicate predicate, ValPtr S1, ValPtr S2, BBPtr parent,
				std::string name, BBPtr InsertAtEnd);

			Predicate getPredicate() const { return predicate; }
			void setPredicate(Predicate P) { predicate = P; }
			bool isEquality() const { return predicate == Predicate::CMP_EQ; }

			/// @brief Methodds for support type inquiry through isa, cast, and dyn_cast
			static bool classof(InstPtr I) 
			{ 
				return I->getOpcode() == Instruction::Opcode::Cmp; 
			}

			/// \brief Print the CmpInst.
			void Print(std::ostringstream& out);
		};

		//===-------------------------------------------------------------===//
		//						AllocaInst Class
		//===-------------------------------------------------------------===//
		/// AllocaInst - an instruction to allocate memory on the stack.
		class AllocaInst final : public UnaryOperator
		{
			TyPtr AllocatedType;
		public:
			// AllocaInst(TyPtr Ty,/* ValPtr ArraySize, */std::string Name = "", InstPtr InsertBefore = nullptr);			
			explicit AllocaInst(TyPtr Ty, BBPtr parent, std::string Name = "", BBPtr InsertAtEnd = nullptr);
			// Out-of-line method.
			~AllocaInst() override;
			static AllocaInstPtr Create(TyPtr Ty, BBPtr parent, std::string Name = "");

			/// getAllocatedType - Return the type that is being allocated by the instruction.
			TyPtr getAllocatedType() const { return AllocatedType; }

			/// \brief Print the AllocaInst.
			void Print(std::ostringstream& out);
		};

		//===-------------------------------------------------------------===//
		//						GetElementPtrInst Class
		//===-------------------------------------------------------------===//
		// GetElementPtrInst - an instruction for type-safe pointer arithmetic
		// to access elements of arrays and structs.
		class GetElementPtrInst final : public Instruction
		{
			GetElementPtrInst(const GetElementPtrInst &GEPI) = delete;
			void init(ValPtr Ptr, std::vector<ValPtr> IdxList);
			void init(ValPtr Ptr, ValPtr Idx0, ValPtr Idx1);
		public:
			/// Constructors - Create a getelementptr instruction with a base pointer and an
			/// list of indices.
			GetElementPtrInst(TyPtr PointeeType, ValPtr Ptr, std::vector<ValPtr> IdxList,
				BBPtr parent, std::string Name = "", InstPtr InsertBefore = nullptr);

			/// Constructors - This constructions is convenience method because two
			/// index getelementptr instructions are so common.
			GetElementPtrInst(TyPtr PointeeType, ValPtr Ptr, ValPtr Idx0, ValPtr Idx1, BBPtr parent, 
				std::string Name = "");
		public:
			static GEPInstPtr Create(TyPtr PointeeType, ValPtr Ptr, std::vector<ValPtr> IdxList, 
				BBPtr parent, std::string Name = "", InstPtr InsertBefore = nullptr);
			static GEPInstPtr Create(TyPtr PointeeType, ValPtr Ptr, ValPtr Idx0, ValPtr Idx1,
				BBPtr parent, std::string Name = "", InstPtr InsertBefore = nullptr);

			ValPtr getPointerOperand();
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
		class CallInst final : public Instruction
		{
		private:
			FuncTypePtr FTy;	
			bool IsIntrisicCall;
		public:
			CallInst(FuncTypePtr FTy, ValPtr Func, std::vector<ValPtr> Args, BBPtr parent,
				std::string Name = "", BBPtr InsertAtEnd = nullptr);
			CallInst(IntrinsicPtr Intr, std::vector<ValPtr> Args, BBPtr parent,
				std::string Name = "", InstPtr InsertBefore = nullptr);

			static CallInstPtr Create(ValPtr Func, std::vector<ValPtr> Args, BBPtr parent, 
				std::string Name = "", InstPtr InsertBefore = nullptr);

			static CallInstPtr CallInst::Create(IntrinsicPtr Intr, std::vector<ValPtr> Args, 
				BBPtr parent, std::string Name = "", InstPtr InsertBefore = nullptr);

			/*static CallInstPtr Create(FuncTypePtr *Ty, ValPtr Func, std::list<ValPtr> Args,
				std::string NameStr, InstPtr InsertBefore = nullptr);*/
			// Out-of-line method.
			~CallInst() override;

			FuncTypePtr getFunctionType() const { return FTy; }

			enum TailCallKind {TCK_None = 0, TCK_Tail = 1, TCK_MustTail = 2, TCK_NoTail = 3 };
			
			bool isIntrinsicCall() const { return IsIntrisicCall; }

			// To Do:
			TailCallKind getTailCallKind() const { return TCK_None; }
			bool isTailCall() const { return true; }
			bool isMustTailCall() const { return true; }
			bool isNoTailCall() const { return true; }
			void setTailCall(bool isTC = true) {}
			void setTailCallKind(TailCallKind TCK) {}

			/// getNumArgOperands - Return the number of call arguments.
			unsigned getNumArgOperands() const { return getNumOperands() - 1; }
			ValPtr getArgOperand(unsigned i) const;
			void setArgOperand(unsigned i, ValPtr v);

			ValPtr getCalledFunction() const { return Operands[0].get(); }

			/// getCalledValue - Get a pointer to the function that is invoked by this
			/// instruction.
			ValPtr getCalledValue() const { return getCalledFunction(); }
			void setCalledFunction(ValPtr Fn) { Operands[0] = Fn; }
			void setCalledFunction(FuncTypePtr Ty, ValPtr Fn) 
			{
				FTy = Ty;
				Operands[0] = Fn;
			}

			// Methods for support type inquiry through isa, cast, and dyn_cast:
			static bool classof(InstPtr I) { return I->getOpcode() == Instruction::Opcode::Call; }

			/// \brief Print the CallInst.
			void Print(std::ostringstream& out);
		};

		//===---------------------------------------------------------------===//
		//						ExtractValueInst Class
		//===---------------------------------------------------------------===//
		// ExtractValueInst - This instruction extracts a struct member or array
		// element value from an aggregate value.
		// <result> = extractvalue <aggregate type> <val>, <idx>{, <idx>}*
		// Example:  <result> = extractvalue {i32, float} %agg, 0
		// 在moses中没有array类型，只有聚合类型，也就是匿名类型和用户自定义类型
		class ExtractValueInst final : public UnaryOperator
		{
		private:
			TyPtr AggregateType;

			ExtractValueInst(const ExtractValueInst &EVI);
			void init(std::vector<unsigned> Idxs, std::string NameStr);

			/// Constructors - Create a extractvalue instruction with a base aggregate
			/// value and a list of indices. The first ctor can optionally insert before
			/// an existing instruction, the second appends the new instruction to the 
			// specified BasicBlock.
			ExtractValueInst(ValPtr Agg, std::vector<unsigned> Idxs, std::string NameStr,
				InstPtr InsertBefore = nullptr);
			ExtractValueInst(ValPtr Agg, std::vector<unsigned> Idxs, std::string NameStr,
				BBPtr InsertAtEnd);
			// allocate space for exactly one operand
			// void *operator new(size_t s){ return User::operator new(s, 1); }
		public:
			// Out-of-line method.
			~ExtractValueInst() override;

			static EVInstPtr Create(ValPtr Agg, std::vector<unsigned> Idxs,
				std::string NameStr = "", InstPtr InsertBefore = nullptr)
			{
				// return new ExtractValueInst(Agg, Idxs, NameStr, InsertBefore);
				return nullptr;
			}

			static EVInstPtr Create(ValPtr Agg, std::vector<unsigned> Idxs,
				std::string NameStr, BBPtr InsertAtEnd = nullptr)
			{
				// return new ExtractValueInst(Agg, Idxs, NameStr, InsertAtEnd);
				return nullptr;
			}

			/// \brief Print the ExtractValueInst.
			void Print(std::ostringstream& out);
		};

		//===---------------------------------------------------------------===//
		//						PHINode Class
		//===---------------------------------------------------------------===//

		// PHINode - The PHINode class is used to represent the magical mystical
		// (神秘的)PHI node, that can not exist in nature, but can be synthesized
		// in a computer scientist's overactive imagination.
		// To Do: PHINode的参数是一系列的pair，PHINode的设计暂时不完整。
		class PHINode final : public Instruction
		{
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

			explicit PHINode(TyPtr Ty, unsigned NumReservedValues, BBPtr parent, 
				std::string NameStr = "", InstPtr InsertBefore = nullptr) :
				Instruction(Ty, Instruction::Opcode::PHI, parent)
			{}
		protected:
			// allocHungoffUses - this is moew complicated than the generic
			// User::allocHungoffUses, because we have to allocate Uses for the incoming
			// values and pointers to the incoming blocks, all in one allocation.
			void allocHungoffUses(unsigned N) {}
		public:
			// Constructors - NumReservedValues is a hint for the number of incoming
			// edges that this phi node will have(use 0 if you really have no idea).
			static PHINodePtr Create(TyPtr Ty, unsigned NumReservedValues, std::string NameStr = "",
				InstPtr InsertBefore = nullptr)
			{
				return nullptr;
			}

			static PHINodePtr Create(TyPtr Ty, unsigned NumReservedValues, std::string NameStr,
				BBPtr InsertAtEnd)
			{
				return nullptr;
			}
			// Block iterator interface. This provides access to the list of incoming
			// basic blocks, which parallels the list of incoming values.

			// getNumIncomingValues - Return the number of incoming edges
			unsigned getNumIncomingValues() const { return getNumOperands(); }

			// getIncomingValue - Return incoming value number x
			ValPtr getIncomingValue(unsigned i) const { return nullptr; }
			void setIncomingValue(unsigned i, ValPtr V) {}
			/// getIncomingBlock - Return incoming basic block number @p i.
			BBPtr getIncomingBlock(unsigned i) const {}

			/// getIncomingBlock - Return incoming basic block corresponding
			// to an operand of the PHI.
			BBPtr getIncomingBlock(const Use &U) const {}
			void setIncomingBlock(unsigned i, BBPtr BB){}
			/// addIncoming - Add an incoming value to the end of the PHI list.
			void addIncoming(ValPtr V, BBPtr BB){}

			/// removeIncomingValue - Remove an incoming value. This is useful if a 
			/// predecessor basic block is deleted. The value removed is returned.
			ValPtr removeIncomingValue(unsigned Idx, bool DeletePHIIfEmpty = true);

			// getBasicBlockIdx - Return the first index of the specified basic block
			// in the value list for this PHI. Returns -1 if no instance.
			int getBasicBlockIndex(BBPtr BB) const{ return 0; }

			ValPtr getIncomingValueForBlock(BBPtr BB) const { return nullptr; }

			/// hasConstantValue - If the specified PHI node always merges together the
			/// same value, return the value, otherwise return null.
			/// 如果某个Incoming block始终是不可达的
			ValPtr hasConstantValue() const;

			/// Methods for support type inquiry through isa, cast and dyn_cast:
			static inline bool classof(InstPtr I)
			{
				return I->getOpcode() == Instruction::Opcode::PHI;
			}

			/// \brief Print the PHINode.
			void Print(std::ostringstream& out);
		};

		//===----------------------------------------------------------------===//
		//						ReturnInst Class
		//===----------------------------------------------------------------===//
		// ReturnInst - Return a value (possibly void), from a function. Execution
		// does not continue in this  function any longer.
		class ReturnInst final : public TerminatorInst
		{
			ReturnInst(const ReturnInst &RI) = delete;
		public:
			// Note: If the Value* passed is of type void then the constructor behave as
			// if it was passed NULL.
			// ReturnInst(ValPtr retVal = nullptr, InstPtr InsertBefore = nullptr);
			ReturnInst(BBPtr parent, ValPtr retVal = nullptr, BBPtr InsertAtEnd = nullptr);
			~ReturnInst() override;
			static RetInstPtr Create(BBPtr parent, ValPtr retVal = nullptr, BBPtr InsertAtEnd = nullptr);

			ValPtr getReturnValue() const { return Operands.empty() ? nullptr : Operands[0].get(); }

			BBPtr getSuccessor(unsigned index) const override
			{
				assert(0 && "ReturnInst has no successor!");
				return 0;
			}
			unsigned getNumSuccessors() const override 
			{
				assert(0 && "ReturnInst has no successor!");
				return 0;
			}
			void setSuccessor(unsigned idx, BBPtr B) override;

			// Method for support type inquiry through isa, cast and dyn_cast:
			static bool classof(InstPtr I)
			{
				return I->getOpcode() == Instruction::Opcode::Ret;
			}

			/// \brief Print the ReturnInst.
			void Print(std::ostringstream& out);
		};

		//===----------------------------------------------------------------===//
		//						BranchInst Class
		//===----------------------------------------------------------------===//

		// BranchInst - Conditional or Unconditional Branch instruction.
		class BranchInst : public TerminatorInst
		{
			BranchInst(const BranchInst &BI) = delete;

			// BranchInst(BB *B)							- 'br B'
			// BranchInst(BB* T, BB *F, Value *C)			- 'br C, T, F'
			// BranchInst(BB *B, Inst *I)					- 'br B'		insert before I
			// BranchInst(BB *T, BB *F, Value *C, Inst *I)	- 'br C, T, F'	insert before I
			// BranchINst(BB *B, BB *I)						- 'br B'		insert at end
			// BranchhInst(BB *T, BB *F, Value *C, BB *I)	- 'br C, T, F'	insert at end			
		public:
			BranchInst(MosesIRContext &Ctx, BBPtr IfTrue, BBPtr parent, BBPtr InsertAtEnd = nullptr);
			BranchInst(MosesIRContext &Ctx, BBPtr IfTrue, BBPtr IfFalse, ValPtr Cond, BBPtr parent, BBPtr InsertAtEnd = nullptr);
			static BrInstPtr Create(MosesIRContext &Ctx, BBPtr IfTrue, BBPtr parent, BBPtr InsertAtEnd = nullptr);
			static BrInstPtr Create(MosesIRContext &Ctx, BBPtr IfTrue, BBPtr IfFalse, ValPtr Cond, BBPtr parent,
				BBPtr InsertAtEnd = nullptr);
			~BranchInst() override;
			bool isUncoditional() const { return getNumOperands() == 1; }
			bool isConditional() const { return getNumOperands() == 3; }
			ValPtr getCondition() const 
			{
				assert(Operands.size() > 1 && "Condition branch instruction must have 3 operands!");
				return Operands[2].get();
			}
			void setCondition(ValPtr V) 
			{
				assert(Operands.size() > 1 && "Condition branch instruction must have 3 operands!");
				Operands[2] = V;
			}
			unsigned getNumSuccessors() const { return 1 + isConditional(); }
			BBPtr getSuccessor(unsigned i) const;
			void setSuccessor(unsigned idx, BBPtr NewSucc);

			// Methods for support type inquiry through isa, cast and dyn_cast:
			static bool classof(InstPtr I)
			{
				return I->getOpcode() == Instruction::Opcode::Br;
			}

			/// \brief Print the BranchInst.
			void Print(std::ostringstream& out);
		};

		//===----------------------------------------------------------------===//
		// LoadInst - an instruction for reading from memory.
		class LoadInst : public UnaryOperator
		{
		public:
			LoadInst(ValPtr Ptr, BBPtr parent, std::string Name = "", BBPtr InsertAtEnd = nullptr);
			~LoadInst() override;
			static LoadInstPtr Create(ValPtr Ptr, BBPtr parent);

			ValPtr getPointerOperand() const 
			{ 
				assert(!Operands.empty() && "Load instruciton's operand may not be null");
				return Operands[0].get(); 
			}

			static bool classof(LoadInstPtr) { return true; }
			static bool classof(InstPtr I)
			{
				return I->getOpcode() == Instruction::Opcode::Load;
			}

			/// \brief Print the LoadInst.
			void Print(std::ostringstream& out);
		};

		//===----------------------------------------------------------------===//
		// StoreInst - an instruction for storing to memory.
		class StoreInst : public Instruction
		{
			void init(ValPtr Val, ValPtr Ptr);
		public:
			// StoreInst(ValPtr Val, ValPtr Ptr, InstPtr InsertBefore = nullptr);
			StoreInst(MosesIRContext &Ctx, ValPtr Val, ValPtr Ptr, BBPtr parent, 
				BBPtr InsertAtEnd = nullptr);

			static StoreInstPtr Create(MosesIRContext &Ctx, ValPtr Val, ValPtr Ptr, BBPtr parent);
			static bool classof(StoreInstPtr) { return true; }
			static bool classof(InstPtr I) { return I->getOpcode() == Instruction::Opcode::Store; }

			/// \brief Print the StoreInst.
			void Print(std::ostringstream& out);
		};
	}
}

#endif