//===----------------------------------BasicBlock.h-----------------------===//
// 
// This file contains the declaration of the BasicBlock class, which represents
// a single basic block in the vm.
//
// Note that well formed basic blocks are formed of a list of instructions
// followed by a single TerminatorInst instruction. TerminatorInst's may not
// occur in the middle of basic blocks, and must terminate the blocks.
//
// This code allows malformed(»ûÐÎµÄ) basix blocks to occur, because it may be 
// useful in the intermediate stage modification to a program.
// 
//===---------------------------------------------------------------------===//
#ifndef MOSES_IR_BASIC_BLOCK_H
#define MOSES_IR_BASIC_BLOCK_H
#include <string>
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
		class BasicBlock : public Value
		{
		private:
			// Instruction List
			std::list<Instruction> InstList;
			BasicBlock *Prev, *Next;
			Function* Parent;		
			void setParent(Function *parent);
			void setNext(BasicBlock *N) { Next = N; }
			void setPrev(BasicBlock *N) { Prev = N; }

			BasicBlock(const BasicBlock&) = delete;
			void operator=(const BasicBlock&) = delete;
		public:
			/// BasicBlock ctor - If the function parameter is specified, the basic block
			/// is automatically inserted at the end of the function.
			BasicBlock(const std::string &Name = "", Function *Parent = 0);
			~BasicBlock();

			// Specialize setName to take care of symbol table majik
			virtual void setName(const std::string &name, SymbolTable *ST = 0);

			// getParent - Return the enclosing method, or null if none
			const Function *getParent() const { return Parent; }
			Function *getParent() { return Parent; }

			// getNext/Prev - Return the next or previous basic block in the list.
			BasicBlock *getNext()       { return Next; }
			const BasicBlock *getNext() const { return Next; }
			BasicBlock *getPrev()       { return Prev; }
			const BasicBlock *getPrev() const { return Prev; }

			/// getTerminator() - If this is a well formed basic block, the this returns 
			/// a pointer to the terminator instruction. It it is not, then you get a 
			/// null pointer back.
			TerminatorInst *getTerminator();
			const TerminatorInst *const getTerminator() const;

			//===--------------------------------------------------------------------===//
			// Instruction iterator methods
			//===--------------------------------------------------------------------===//

			const std::list<Instruction> &getInstList() const { return InstList; }
			std::list<Instruction> &getInstList() { return InstList; }

			// methods for support type inquiry thorough isa, cast, and dyn_cast
			static bool classof(const BasicBlock*) { return true; }
			static bool classof(const Value* V)
			{
				return V->getValueType() == Value::ValueTy::BasicBlockVal;
			}
		};
	}
}
#endif