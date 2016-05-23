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
#include "User.h"
namespace compiler
{
	namespace IR
	{
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

				Alloca,			// Stack management
				Load,			// Memory manipulation instrs load
				Store,			// Memory mainpulation instrs store
				GetElementPtr,	// Struct Type's intrs

				Trunc,			// anonymous type cast to struct types
				Cmp,			// Integer comparison instruciton
				PHI,			// PHI node instruction
				Call			// Call a function
			};
			typedef std::shared_ptr<Instruction> InstPtr;
			typedef std::shared_ptr<BasicBlock> BBPtr;

		private:
			BBPtr Parent;
			InstPtr Prev, Next;
			Opcode op;

			void setNext(InstPtr N) { Next = N; }
			void setPrev(InstPtr P) { Prev = P; }

			void setParent(BBPtr P);
		public:
			BBPtr getParent() const { return Parent; }

			/// Return the function this instruction belongs to.
			/// 
			/// Note: it is undefined behavior to call this on an instruction not
			/// currently inserted into a function.
			const Function *getFunction() const;
			Function *getFunction();

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

			bool isTerminator() const 
			{ 
				return op == Opcode::Ret || op == Opcode::Br;
			}

			bool isBinaryOp() const 
			{ 
				return op >= Opcode::Add && op <= Opcode::Or;
			}

			bool isShift() 
			{ 
				return op == Opcode::Shl || op == Opcode::Shr;
			}

			bool isCast() const 
			{ 
				return op == Opcode::Trunc;
			}

			//===---------------------------------------------------------===//
			// Predicates and helper methods.
			//===---------------------------------------------------------===//
			
			// Return true if the intruction is associative:
			//
			// Associative operators satisfy: x op (y op z) == (x op y) op z
			//
			// In moses IR, the Add, Mul, Mul, and Xor operators are associative.
			bool isAssociative() const;
			
			// Return true if the instruction is commutative:
			// Commutative operators satisfy: (x op y) == (y op x)
			
			bool isCommutative() const;

			// Return true if this instruction may modify memory.
			bool mayWriteToMemory() const;

			// Return true if this memory may read memory.
			bool mayReadFromMemory() const;

			bool mayReadOrWriteMemory() const
			{
				// return mayReadRromMemory() || mayWriteToMemory();
				return true;
			}

			// Return true if the instruciton may have side effects.
			// Note that this doest not consider malloc and alloca to have side 
			// effects because the newly allocated memory is completely invisible
			// to instructions which don't use the returned value.
			bool mayHaveSideEffects() const;
		public:
			Instruction(TyPtr Ty, Opcode op, unsigned NumOps, InstPtr InsertBefore = nullptr);
			Instruction(TyPtr Ty, Opcode op, unsigned NumOps, BBPtr InsertAtEnd = nullptr);
		};

		//===-------------------------------------------------------------===//
		//						UnaryInstruction Class
		//===-------------------------------------------------------------===//

		// ---------------------nonsense for coding------------------------- //
		// 对于C++来说new有两层含义，
		// (1) new operator: Base * b = new base();
		//	  这里的new表示的是new operator，有两个操作，分配内存调用构造函数，不能
		//    重载，和C语言中的sizeof相似，
		// (2) void* operator new(size_t size) {}
		//    这里的new表示的是一个内存分配函数，分配size字节大小的内存，可以重载，
		//    这里的new类似于C语言中的malloc，对构造函数一无所知，返回的是raw的内存
		// ---------------------nonsense for coding------------------------- //
		class UnaryInstruction : public Instruction
		{
		private:
			UsePtr operand0;
		protected:
			UnaryInstruction(TyPtr Ty, Opcode op, ValPtr V, InstPtr IB = nullptr)
				: Instruction(Ty, op, 1, IB) 
			{
				// set operands

			}
		public:

			static bool classof(InstPtr I)
			{
				I->getOpcode() == Instruction::Opcode::Alloca;
				return I->getOpcode() == Instruction::Opcode::Alloca || I->getOpcode() == Instruction::Opcode::Load;
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
			TerminatorInst(TyPtr Ty, Instruction::Opcode op, /*Use *Ops*/ unsigned NumOps,
				InstPtr InsertBefore = nullptr) : Instruction(Ty, op, NumOps, InsertBefore)
			{}

			TerminatorInst(TyPtr Ty, Instruction::Opcode op, /*Use *Ops*/ unsigned NumOps,
				BBPtr InsertAtEnd = nullptr) : Instruction(Ty, op, NumOps, InsertAtEnd)
			{}

			// Out of line virtual method, so the vtable, etc has a home.
			~TerminatorInst() override;

			/// Virtual methods - Terminators should overload these and provide inline
			/// overrides of non-V methods.
			virtual BBPtr getSuccessorV(unsigned idx) const = 0;
			virtual unsigned getNumSuccessorV() const = 0;
			virtual void setSuccessorV(unsigned idx, BBPtr B) = 0;
		public:
			/// Return the number of successors that this terminator hsa.
			unsigned getNumSuccessors() const
			{
				return getNumSuccessorV();
			}

			/// Return the specified successor.
			BBPtr getSuccessor(unsigned idx) const
			{
				return getSuccessorV(idx);
			}

			/// Update the specified successor to point at the provided block.
			void setSuccessor(unsigned idx, BBPtr B)
			{
				setSuccessorV(idx, B);
			}

			// Methods for support type inquiry through isa, cast, and dyn_cast:
			static bool classof(InstPtr I)
			{
				return I->isTerminator();
			}
		};

		//===-------------------------------------------------------------===//
		//						BinaryOperator Class
		//===-------------------------------------------------------------===//
		class BinaryOperator : public Instruction
		{
		public:
			typedef std::shared_ptr<BinaryOperator> BOPtr;
		private:
			UsePtr operand0, operand1;
		protected:
			void init();
			BinaryOperator(Opcode op, ValPtr S1, ValPtr S2, TyPtr Ty, 
				InstPtr InstructionBefore = nullptr);

			BinaryOperator(Opcode op, ValPtr S1, ValPtr S2, TyPtr Ty, BBPtr InsertAtEnd = nullptr);
		public:
			// allocate space for exactly two operands.

			/// Construct a binary instruction, given the opcode and the two operands.
			/// Optionally (if InstBefore is specified) insert the instruction into a 
			/// BasicBlock right before the specified instruction.
			static BOPtr Create(Opcode op, ValPtr S1, ValPtr S2, InstPtr InsertBefore = nullptr);

			/// Construct a binary instruction, given the opcode and the two operands.
			/// Also automatically insert this instruction to the end of the BasicBlock
			/// specified.
			static BOPtr Create(Opcode op, ValPtr S1, ValPtr S2, BBPtr InsertAtEnd = nullptr);

			/// These methods just forward to Create, and are useful when you statically
			/// know what type of instruction you're going to create. These just save 
			/// some typing.


			// Methods for support type inquiry through isa, cast, and dyn_cast:
			static bool classof(InstPtr I)
			{
				return I->isBinaryOp();
			}
		};

		//===-------------------------------------------------------------===//
		//						CastInst Class
		//===-------------------------------------------------------------===//
		// This is the base class for all instructions that perform data casts.
		// 在moses中唯一需要执行 cast 操作的就是匿名类型赋值给用户自定义类型的转换
		class CastInst : public UnaryInstruction
		{

		};

		//===-------------------------------------------------------------===//
		//						CmpInst Class
		//===-------------------------------------------------------------===//

		class CmpInst : public Instruction
		{
		public:
			typedef std::shared_ptr<CmpInst> CmpInstPtr;
			/// This enumeration lists the possible predicates for CmpInst.
			enum Predicate
			{
				// Opcode
				CMP_FALSE,
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
		protected:
			CmpInst(InstPtr InsertBefore,		/// Where to insert
				Predicate pred,					/// The predicate to use for the comparison
				ValPtr LHS,						/// The left-hand-side of the expression
				ValPtr RHS,						/// The right-hand-side of the expression
				std::string NameStr = "");		/// Name of the instruction

			// allocate space for exactly two operands

			/// Construct a compare instruction, given the opcode, the predicaee and
			/// the two operands. Optionally (if InstBefore is specified) insert the
			/// instruction into a BasicBlock right before the specified instruction
			/// @brief Create a CmpInst
			static CmpInstPtr Create(Predicate predicate, ValPtr S1, ValPtr S2,
				std::string name = "", InstPtr InsertBefore = nullptr);

			static CmpInstPtr Create(Predicate predicate, ValPtr S1, ValPtr S2,
				std::string name, BBPtr InsertAtEnd);

			Predicate getPredicate() const { return predicate; }

			void setPredicate(Predicate P) { predicate = P; }

			bool isCommutative() const;
			bool isEquality() const;

			/// @brief Methodds for support type inquiry through isa, cast, and dyn_cast
			static bool classof(InstPtr I) 
			{ 
				return I->getOpcode() == Instruction::Opcode::Cmp; 
			}
		};


		//===-------------------------------------------------------------===//
		//						AllocaInst Class
		//===-------------------------------------------------------------===//

		/// AllocaInst - an instruction to allocate memory on the stack
		/// 在moses中，只有单值的alloca，暂时不支持Array的alloca
		class AllocaInst : public UnaryInstruction
		{
		public:
			typedef std::shared_ptr<AllocaInst> AllocInstPtr;
		private:
			TyPtr AllocatedType;
		public:
			explicit AllocaInst(TyPtr Ty, InstPtr InsertBefore = nullptr);

			AllocaInst(TyPtr Ty, BBPtr InsertAtEnd);

			// Out of line virtual method, so the vtable, etc. has a home.
			~AllocaInst() override;

			// 有问题参考LLVM
			TyPtr getType() const 
			{
				return nullptr;
			}

			/// getAllocatedType - Return the type that is being allocated by the
			/// instruction.
			TyPtr getAllocatedType() const { return AllocatedType; }
		};

		//===-------------------------------------------------------------===//
		//						GetElementPtrInst Class
		//===-------------------------------------------------------------===//

		// GetElementPtrInst - an instruction for type-safe pointer arithmetic
		// to access elements of arrays and structs.

		class GetElementPtrInst : public Instruction
		{
		public:
			typedef GetElementPtrInst GEPInstPtr;
		private:
			TyPtr SourceElementType;
			TyPtr ResultElementType;

			GetElementPtrInst(const GetElementPtrInst &GEPI);
			void init(ValPtr Ptr, std::list<ValPtr> IdxList);

			/// Constructors - Create a getelementptr instructions with a base pointer an
			/// list of indices. The first ctor can optionally insert before an existing
			/// instruction, the second appends the new instruction to the specified
			/// BasicBlock.
			GetElementPtrInst(TyPtr PointeeType, ValPtr Ptr, std::list<ValPtr> IdxList,
				unsigned Values, InstPtr InsertBefore = nullptr);

			GetElementPtrInst(TyPtr PointeeType, ValPtr Ptr, std::list<ValPtr> IdxList,
				unsigned Values, BBPtr InsertAtEnd);
			
		protected:
			static GEPInstPtr Create(TyPtr PointeeType, ValPtr Ptr,
				std::list<ValPtr> IdxList, InstPtr InsertBefore = nullptr)
			{
				unsigned Values = 1 + unsigned(IdxList.size());
				// 进行类型检查
				/*return new (Values) GetElementPtrInst(PointeeType, Ptr, IdxList, Values, 
					InsertBefore);*/
			}

			// getType - Overload to return most specific sequential type.
			// SequentialType *getType() const 
			// {
			//		return cast<SequaentialType>(Instruction::getType());
			// }

			TyPtr getSourceElementType() const { return SourceElementType; }

			void setSourceElementType(TyPtr Ty) { SourceElementType = Ty; }
			void setResultElementType(TyPtr Ty) { ResultElementType = Ty; }

			/// getIndexedType

			static TyPtr getIndexedType(TyPtr Ty, std::list<ValPtr> IdxList);
			// ...

			ValPtr getPointerOperand()
			{
				// return nullptr;
			}

			const ValPtr getPointerOperand() const
			{
				// return nullptr;
			}

			static unsigned getPointerOperandIndex()
			{}
			// .. 补全
		};		

		//===-------------------------------------------------------------===//
		//						CallInst
		//===-------------------------------------------------------------===//
		
		// ClassInst - This class represents a function call, abstracting a 
		// target mechine's calling convention. This class uses low bit of 
		// the SubClassData field to indicate whether or not this is a tail
		// call. The rest of the bits hold the calling convention of the call.

		class CallInst : public Instruction
		{
		public:
			typedef std::shared_ptr<CallInst> CallInstPtr;
			typedef std::shared_ptr<FunctionType> FuncTyPtr;
			typedef std::shared_ptr<Function> FunctionPtr;
		private:
			FuncTyPtr FTy;

			void init(FuncTyPtr FTy, ValPtr Func, std::list<ValPtr> Args,
					std::string NameStr);
			void init(ValPtr Func, std::string NameStr);

			/// Construct a CallInst given a range of arguments.
			CallInst(FuncTyPtr Ty, ValPtr Func, std::list<ValPtr> Args, 
					std::string NameStr);
			CallInst(ValPtr Func, std::list<ValPtr> Args, 					
				std::string NameStr, InstPtr InsertBefore = nullptr);
		public:
			static CallInstPtr Create(ValPtr Func, std::list<ValPtr> Args, std::string NameStr = "",
				InstPtr InsertBefore = nullptr)
			{
				// return nullptr;
			}

			static CallInstPtr Create(FuncTyPtr *Ty, ValPtr Func, std::list<ValPtr> Args,
				std::string NameStr, InstPtr InsertBefore = nullptr)
			{
				// return nullptr;
			}

			static InstPtr CreateMalloc();

			~CallInst() override;

			FuncTyPtr getFunctionType() const { return FTy; }

			enum TailCallKind {TCK_None = 0, TCK_Tail = 1, TCK_MustTail = 2, TCK_NoTail = 3 };

			TailCallKind getTailCallKind() const
			{
				// return TailCallKind();
				return TCK_None;
			}

			bool isTailCall() const { return true; }

			bool isMustTailCall() const { return true; }

			bool isNoTailCall() const { return true; }

			void setTailCall(bool isTC = true) {}

			void setTailCallKind(TailCallKind TCK) {}

			/// getNumArgOperands - Return the number of call arguments.
			unsigned getNumArgOperands() const { return 10; }

			ValPtr getArgOperand(unsigned i) const { }

			void setArgOperand(unsigned i, ValPtr v) {}

			/// \brief Determine if the call does not access memory.
			bool doesNotAccessMemory() const
			{
				return true;
			}

			void setDoesNotAccessMemory() {}

			/// \brief Determine if the call does not access or only reads memory.
			bool onlyReadsMemory() const { return true; }

			void setOnlyReadsMemory() {}

			/// @brief Determine if the call can access memmory only using pointers
			/// based on its arguments.
			bool onlyAccessesArgMemory() const { return false; }

			void setOnlyAccessesArgMemory() {}

			/// \brief Determine if any call argument is an aggregate passed by value.
			bool hasByValArgument() const { return true; }

			FunctionPtr getCalledFunction() const 
			{ 
				// return nullptr; 
			}

			/// getCalledValue - Get a pointer to the function that is invoked by this
			/// instruction.
			ValPtr getCalledValue() 
			{ 
				// return nullptr; 
			}

			void setCalledFunction(ValPtr Fn) {}

			void setCalledFunction(FuncTyPtr FTy, ValPtr Fn) {}

			// Methods for support type inquiry through isa, cast, and dyn_cast:
			static bool classof(InstPtr I)
			{
				return I->getOpcode == Instruction::Opcode::Call;
			}
		};

		//===---------------------------------------------------------------===//
		//						ExtractValueInst Class
		//===---------------------------------------------------------------===//

		// ExtractValueInst - This instruction extracts a struct member or array
		// element value from an aggregate value.
		// 在moses中没有array类型，只有聚合类型，也就是匿名类型和用户自定义类型
		class ExtractValueInst : public UnaryInstruction
		{
		public:
			typedef ExtractValueInst EVInstPtr;
		private:
			std::vector<unsigned> Indices;

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
			void *operator new(size_t s){ return User::operator new(s, 1); }
		public:
			static EVInstPtr Create(ValPtr Agg, std::vector<unsigned> Idxs,
				std::string NameStr = "", Instruction *InsertBefore = nullptr)
			{
				// return new ExtractValueInst(Agg, Idxs, NameStr, InsertBefore);
			}

			static EVInstPtr Create(ValPtr Agg, std::vector<unsigned> Idxs,
				std::string NameStr, BBPtr InsertAtEnd)
			{
				// return new ExtractValueInst(Agg, Idxs, NameStr, InsertAtEnd);
			}

			/// getIndexedType - 
		};

		//===---------------------------------------------------------------===//
		//						PHINode Class
		//===---------------------------------------------------------------===//

		// PHINode - The PHINode class is used to represent the magical mystical
		// (神秘的)PHI node, that can not exist in nature, but can be synthesized
		// in a computer scientist's overactive imagination.
		class PHINode : public Instruction
		{
		public:
			typedef std::shared_ptr<PHINode> PHINodePtr;
		private:
			// void *operator new(size_t, unsigned) = delete;
			/// ReservedSpace - The number of operands actually allocated. NumOperands
			/// is the number actually in use.
			unsigned ReservedSpace;
			PHINode(const PHINode &PN);

			// allocate space for exactly zero operands
			/*void *operator new(size_t s)
			{
				return User::operator new(s);
			}*/

			explicit PHINode(TyPtr Ty, unsigned NumReservedValues, 
				std::string NameStr = "", InstPtr InsertBefore = nullptr) :
				Instruction(Ty, Instruction::Opcode::PHI, 0, InsertBefore),
				ReservedSpace(NumReservedValues)
			{
			}
		protected:
			// allocHungoffUses - this is moew complicated than the generic
			// User::allocHungoffUses, because we have to allocate Uses for the incoming
			// values and pointers to the incoming blocks, all in one allocation.
			void allocHungoffUses(unsigned N)
			{}
		public:
			// Constructors - NumReservedValues is a hint for the number of incoming
			// edges that this phi node will have(use 0 if you really have no idea).
			static PHINodePtr Create(TyPtr Ty, unsigned NumReservedValues, std::string NameStr = "",
				InstPtr InsertBefore = nullptr)
			{
				// return new 
				// return nullptr;
			}

			static PHINodePtr Create(TyPtr Ty, unsigned NumReservedValues, std::string NameStr,
				BBPtr InsertAtEnd)
			{
				// return new 
				return nullptr;
			}

			// Block iterator interface. This provides access to the list of incoming
			// basic blocks, which parallels the list of incoming values.

			// ...

			// getNumIncomingValues - Return the number of incoming edges
			unsigned getNumIncomingValues() const { return getNumOperands(); }

			// getIncomingValue - Return incoming value number x
			ValPtr getIncomingValue(unsigned i) const \
			{ 
				// return getOperand(i); 
				return nullptr;
			}

			void setIncomingValue(unsigned i, ValPtr V)
			{
				
			}

			/// getIncomingBlock - Return incoming basic block number @p i.
			BBPtr getIncomingBlock(unsigned i) const
			{
				// return block_begin()[i];
				// return nullptr;
			}

			/// getIncomingBlock - Return incoming basic block corresponding
			// to an operand of the PHI.
			BBPtr getIncomingBlock(const Use &U) const
			{
				// return nullptr;
			}

			void setIncomingBlock(unsigned i, BBPtr BB)
			{

			}

			/// addIncoming - Add an incoming value to the end of the PHI list.
			void addIncoming(ValPtr V, BBPtr BB)
			{}

			/// removeIncomingValue - Remove an incoming value. This is useful if a 
			/// predecessor basic block is deleted. The value removed is returned.
			ValPtr removeIncomingValue(unsigned Idx, bool DeletePHIIfEmpty = true);

			// getBasicBlockIdx - Return the first index of the specified basic block
			// in the value list for this PHI. Returns -1 if no instance.
			int getBasicBlockIndex(BBPtr BB) const
			{
				return 0;
			}

			ValPtr getIncomingValueForBlock(BBPtr BB) const
			{
				// return nullptr;
			}

			/// hasConstantValue - If the specified PHI node always merges together the
			/// same value, return the value, otherwise return null.
			/// 如果某个Incoming block始终是不可达的
			ValPtr hasConstantValue() const;

			/// Methods for support type inquiry through isa, cast and dyn_cast:
			static inline bool classof(InstPtr I)
			{
				return I->getOpcode() == Instruction::Opcode::PHI;
			}
		};

		//===----------------------------------------------------------------===//
		//						ReturnInst Class
		//===----------------------------------------------------------------===//

		// ReturnInst - Return a value (possibly void), from a function. Execution
		// does not continue in this  function any longer.
		class ReturnInst : public TerminatorInst
		{
		public:
			typedef std::shared_ptr<ReturnInst> RetInstPtr;
		private:
			ReturnInst(const ReturnInst &RI) = delete;
			ValPtr RetVal;
		private:
			// ReturnInst constructors:
			// ReturnInst()						- 'ret void'	instruction
			// ReturnInst(	null)				- 'ret void'	instruciton
			// ReturnInst(Value *X)				- 'ret X'		instruction
			// ReturnInst(	null, Inst *I)		- 'ret void'	instruction, insert before I
			// ReturnInst(Value *X, Inst *I)	- 'ret X'		instruction, insert before I
			// ReturnInst(	null, BB *B)		- 'ret void'	instruction, insert @ end of B
			// ReturnInst(Value *X, BB *B)		- 'ret X'		instruction, insert @ end of B
			// 
			// Note: If the Value* passed is of type void then the constructor behave as
			// if it was passed NULL.
			ReturnInst(ValPtr retVal, InstPtr InsertBefore = nullptr);

			ReturnInst(ValPtr retVal, BBPtr InsertAtEnd);
		public:
			static RetInstPtr Create(ValPtr retVal = nullptr, InstPtr InsertBefore = nullptr)
			{
				// return new(!!retVal)
				return nullptr;
			}

			static RetInstPtr Create(ValPtr retVal, BBPtr InsertAtEnd)
			{
				// return new
				// return nullptr;
			}

			static RetInstPtr Create(BBPtr InsertAtEnd)
			{
				// return nullptr;
			}

			~ReturnInst();

			ValPtr getReturnValue() const
			{
				return RetVal;
			}

			unsigned getNumSuccessors() const { return 0; }

			// Method for support type inquiry through isa, cast and dyn_cast:
			static bool classof(InstPtr I)
			{
				return I->getOpcode() == Instruction::Opcode::Ret;
			}
		};

		//===----------------------------------------------------------------===//
		//						BranchInst Class
		//===----------------------------------------------------------------===//

		// BranchInst - Conditional or Unconditional Branch instruction.
		class BranchInst : TerminatorInst
		{
			/// Ops list - Branches are strange. The operands are ordered:
			/// [Cond, FalseDest, ] True Dest. This make some accessors faster
			/// because they don't have to check for cond/uncond branchness. These 
			/// are mostly accessed relative from op_end().
			BranchInst(const BranchInst &BI);

			// BranchInst constructors (where {B, T, F} are blocks, and C is a condition):
			// BranchInst(BB *B)							- 'br B'
			// BranchInst(BB* T, BB *F, Value *C)			- 'br C, T, F'
			// BranchInst(BB *B, Inst *I)					- 'br B'		insert before I
			// BranchInst(BB *T, BB *F, Value *C, Inst *I)	- 'br C, T, F'	insert before I
			// BranchINst(BB *B, BB *I)						- 'br B'		insert at end
			// BranchhInst(BB *T, BB *F, Value *C, BB *I)	- 'br C, T, F'	insert at end
			explicit BranchInst(BBPtr IfTrue, InstPtr InsertBefore = nullptr);

			BranchInst(BBPtr IfTrue, BBPtr IfFalse, ValPtr Cond,
				InstPtr InsertBefore = nullptr);

			BranchInst(BBPtr IfTrue, BBPtr InsertAtEnd);

			BranchInst(BBPtr IfTrue, BBPtr IfFalse, ValPtr Cond,
				BBPtr InsertAtEnd);
		public:
			static BBPtr Create(BBPtr IfTrue, InstPtr InsertBefore = nullptr)
			{
				return nullptr;
				// return new
			}

			static BBPtr Create(BasicBlock *IfTrue, BBPtr IfFalse, ValPtr Cond,
				InstPtr InsertBefore = nullptr)
			{
				return nullptr;
			}

			static BBPtr Create(BBPtr IfTrue, BBPtr IfFalse, ValPtr Cond,
				BBPtr InsertAtEnd)
			{
				return nullptr;
				//return new
			}

			bool isUncoditional() const 
			{
				return true;
				// return getNumOperands() == 1;
			}

			bool isConditional() const
			{
				// return getNumOperands() == 2;
				return true;
			}

			Value* getCondition() const
			{
				return nullptr;
			}

			void setCondition(Value *V)
			{
			
			}

			unsigned getNumSuccessors() const { return 1 + isConditional(); }

			BBPtr getSuccessor(unsigned i) const
			{
				return nullptr;
			}

			void setSuccessor(unsigned idx, BBPtr NewSucc)
			{

			}

			// Methods for support type inquiry through isa, cast and dyn_cast:
			static bool classof(InstPtr I)
			{
				return I->getOpcode() == Instruction::Opcode::Br;
			}
		};

		//===----------------------------------------------------------------===//
		//						IndirectBrInst Class
		//===----------------------------------------------------------------===//

		// IndirectBrInst - Indirect Branch Instruction.
		class IndirectBrInst
		{

		};
	}
}

#endif