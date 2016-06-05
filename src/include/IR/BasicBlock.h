//===----------------------------------BasicBlock.h-----------------------===//
// 
// This file contains the declaration of the BasicBlock class, which represents
// a single basic block in the vm.
//
// Note that well formed basic blocks are formed of a list of instructions
// followed by a single TerminatorInst instruction. TerminatorInst's may not
// occur in the middle of basic blocks, and must terminate the blocks.
//
// This code allows malformed(畸形的) basix blocks to occur, because it may be 
// useful in the intermediate stage modification to a program.
// 
//===---------------------------------------------------------------------===//
#ifndef MOSES_IR_BASIC_BLOCK_H
#define MOSES_IR_BASIC_BLOCK_H
#include <string>
#include <memory>
#include "Instruction.h"

namespace compiler
{
	namespace IR
	{
		class TerminatorInst;
		/// \brief moses IR(LLVM) Basic Block Representation
		/// This represents a single basic block in LLVM. A basic block is simply a 
		/// container of instructions that execute sequentially. Basic blocks are Values
		/// because they are referenced by instructions such as branches. The type of a
		/// BasicBlock is "Type::LabelTy" because the basic block represents a label to
		/// which a branch can jump.
		// Note: BasicBlock与symbol table不是直接挂钩的
		class BasicBlock : public Value
		{
		private:
			// Instruction List
			std::list<InstPtr> InstList;
			BBPtr Prev, Next;
			FuncPtr Parent;		
			void setParent(FuncPtr parent);
			void setNext(BBPtr N) { Next = N; }
			void setPrev(BBPtr N) { Prev = N; }

			BasicBlock(const BasicBlock&) = delete;
			void operator=(const BasicBlock&) = delete;
		public:
			/// BasicBlock ctor - If the function parameter is specified, the basic block
			/// is automatically inserted at the end of the function.
			BasicBlock(std::string Name = "", FuncPtr Parent = nullptr, BBPtr = nullptr);
			~BasicBlock() {}

			/// \brief Creates a new BasicBlock.
			///
			/// If the Parent parameter is specified, the basic block is automatically
			/// inserted at either the end of the function (if InsertBefore is 0), or
			/// before the specified basic block.
			static BBPtr Create(std::string Name = "", FuncPtr = nullptr, BBPtr = nullptr);

			// Specialize setName to take care of symbol table majik
			virtual void setName(std::string name, SymTabPtr ST = nullptr);

			// getParent - Return the enclosing method, or null if none
			FuncPtr getParent() { return Parent; }

			/// removePredecessor - This method is used to notify a BasicBlock that the
			/// specified predicesoor of the block is no longer able to reach it. This is
			/// actully not used to update the Predecessor list, but it is actully used to
			/// update the PHI nodes that reside in the block. Note that this should be 
			/// called while the predecessor still refers to this block.
			void removePredecessor(BBPtr Pred);

			/// splitBasicBlock - This splits a basic block into two the specified
			/// instruction. Note that all instructions BEFORE the specified iterator
			/// stay as part of the original basic block, an unconditional branch is
			/// added to the new BB, and the rest of the instructions in the BB are
			/// moved to the new BB, including the old terminator. The newly formed
			/// BasicBlock is returned.
			BBPtr splitBasicBlock(unsigned index, std::string BBName = "");

			// getNext/Prev - Return the next or previous basic block in the list.
			BBPtr getNext()       { return Next; }
			BBPtr getPrev()       { return Prev; }

			/// getTerminator() - If this is a well formed basic block, the this returns 
			/// a pointer to the terminator instruction. It it is not, then you get a 
			/// null pointer back.
			InstPtr getTerminator();

			//===--------------------------------------------------------------------===//
			// Instruction iterator methods
			//===--------------------------------------------------------------------===//
			std::list<InstPtr> &getInstList() { return InstList; }

			// methods for support type inquiry thorough isa, cast, and dyn_cast
			static bool classof(ValPtr V)
			{
				return V->getValueType() == Value::ValueTy::BasicBlockVal;
			}
		};
	}
}
#endif