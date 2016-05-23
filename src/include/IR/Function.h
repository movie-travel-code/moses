//===---------------------------------Function.h--------------------------===//
//
// This file contains the declaration of the Function class, which represents
// a single function/procedure in moses IR.
//
// A function basically consists of a list of basic blocks, a list of arguments,
// and a symbol table.
//
//===---------------------------------------------------------------------===//
#ifndef MOSES_IR_FUNCTION_H
#define MOSES_IR_FUNCTION_H
#include "ConstantAndGlobal.h"
#include <list>
#include "BasicBlock.h"

namespace compiler
{
	namespace IR
	{
		class FunctionType;

		class Function : public GlobalValue
		{
		public:
			// argument
			// symbol table
		private:
			// Important things that make up a function!
			std::list<BasicBlock> BasicBlocks;
			std::list<Argument> Arguments;

			SymbolTable *SymTab;
			// LLVM 1.0 中指令集什么的都是使用链表来索引的
			Function *Prev, *Next;
			void setNext(Function *N) { Next = N; }
			void setPrev(Function* N) { Prev = N; }
		public:
			Function(const FunctionType* Ty, const std::string &N = "");
			~Function();

			// Specialize setName to handle symbol table majik...
			virtual void setName(const std::string &name, SymbolTable *ST = 0);

			const Type *getReturnType() const;			 // Return the type of the ret val
			const FunctionType *getFunctionType() const; // Return the FunctionType for me

			/// getIntrinsicID - This method returns the ID number of the specified function.
			/// This value is always defined to be zero to allow easy checking for whether
			// a function if intrinsic or not. The paticular intrinsic functions which
			/// correspond to this value are defined in Intrinsics.h
			unsigned getIntrinsicID() const;
			bool isIntrinsic() const { return getIntrinsicID() != 0; }

			// getNext/Prev - Return the next or previous function in the list. These methods
			// should never be used directly, and are only used to implement the function list
			// as part of the module.
			Function *getNext()	{ return Next; }
			const Function *getNext() const { return Next; }
			Function *getPrev()	{ return Prev; }
			const Function *getPrev() const { return Prev; }

			/// Get the underlying elements of the Function... the basic block list is empty
			/// for external functions.
			const std::list<Argument> &getArgumentList() const { return Arguments; }
			std::list<Argument> &getArgumentList() { return Arguments; }

			const std::list<BasicBlock> &getBasicBlockList() const { return BasicBlocks; }
			std::list<BasicBlock> &getBasicBlockList() { return BasicBlocks; }

			// const BasicBlock &getEntryBlock() const {}

			//===-------------------------------------------------------------------===//
			// Symbol Table Accessing functions...
			SymbolTable &getSymbolTable() { return *SymTab; }
			const SymbolTable &getSymbolTable() const { return *SymTab; }

			/// Determine if the function is known not to recurse, directly or
			/// indirectly.
			bool doesNotRecurse() const
			{
				return true;
			}

			static bool classof(const Function*) { return true; }
			static bool classof(const Value *V)
			{
				return V->getValueType() == Value::ValueTy::FunctionVal;
			}

			void dropAllReferences();

			bool doesNotAccessMemory(unsigned n)
			{
			}

			void setDoseNotAccessMemory(unsigned n)
			{
			}

			bool onlyReadsMemory(unsigned n) const
			{
			}

			void setOnlyReadsMemory(unsigned n)
			{
			}

			/// Optimize this function for minimum size (-Oz).
			bool optForMinSize() const
			{
			}

			/// Optimize this function for size (-Os) or minimum size (-Oz).
			bool optForSize() const
			{
			}

			//===----------------------------------------------------------===//
			// Symbol Table Accessing functions...
			// ValueSymbolTable& getValueSymbolTable() {return *SymTab;}
		};


		/// \brief moses IR(LLVM) Argument representation.
		///
		/// This class represents an incoming formal argument to a Function. A formal
		/// argument, since it is 'formal', does not contain an actual value but instead
		/// represents the type, arguement number, and attributes of an argument for a 
		/// specific function. When used in the body of said funciton, the argument of
		/// course represents the value of the actual argument that the function was
		/// called with.
		class Argument : public Value
		{
			void setParent(Function* parent);
		public:

		};
	}
}

#endif
