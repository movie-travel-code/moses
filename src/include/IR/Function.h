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
#include <list>
#include <string>
#include "ConstantAndGlobal.h"
#include "BasicBlock.h"
#include "ValueSymbolTable.h"

namespace compiler
{
	namespace IR
	{
		class Function : public GlobalValue
		{
		private:
			// Important things that make up a function!
			std::list<BBPtr> BasicBlocks;
			std::list<ArgPtr> Arguments;

			SymTabPtr SymTab;
			// LLVM 1.0 中指令集什么的都是使用链表来索引的
			FuncPtr Prev, Next;
			void setNext(FuncPtr N) { Next = N; }
			void setPrev(FuncPtr N) { Prev = N; }
		public:
			Function(FuncTypePtr Ty, const std::string &N = "");
			~Function();

			// Specialize setName to handle symbol table majik...
			virtual void setName(const std::string &name, SymTabPtr ST = nullptr);

			TyPtr getReturnType() const;			 // Return the type of the ret val
			FuncTypePtr getFunctionType() const; // Return the FunctionType for me

			/// getIntrinsicID - This method returns the ID number of the specified function.
			/// This value is always defined to be zero to allow easy checking for whether
			// a function if intrinsic or not. The paticular intrinsic functions which
			/// correspond to this value are defined in Intrinsics.h
			unsigned getIntrinsicID() const;
			bool isIntrinsic() const { return getIntrinsicID() != 0; }

			// getNext/Prev - Return the next or previous function in the list. These methods
			// should never be used directly, and are only used to implement the function list
			// as part of the module.
			FuncPtr getNext() { return Next; }
			FuncPtr getPrev()	{ return Prev; }

			/// Get the underlying elements of the Function... the basic block list is empty
			/// for external functions.
			const std::list<ArgPtr> &getArgumentList() const { return Arguments; }
			std::list<ArgPtr> &getArgumentList() { return Arguments; }

			std::list<BBPtr> &getBasicBlockList() { return BasicBlocks; }

			// const BasicBlock &getEntryBlock() const {}

			//===-------------------------------------------------------------------===//
			// Symbol Table Accessing functions...
			SymTabPtr getSymbolTable() { return SymTab; }

			/// Determine if the function is known not to recurse, directly or
			/// indirectly.
			bool doesNotRecurse() const
			{
				return true;
			}

			static bool classof(ValPtr V)
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
			FuncPtr Parent;
			ArgPtr Prev, Next;
			void setNext(ArgPtr N) { Next = N; }
			void setPrev(ArgPtr N) { Prev = N; }

			void setParent(FuncPtr parent);
		public:
			/// Argument ctor - If Function argument is specified, this argument is inserted at
			/// the end of the argument list for the function.
			Argument(TyPtr Ty, std::string Name = "", FuncPtr F = nullptr);

			/// setName - Specialize setName to handle symbol table majik...
			virtual void setName(std::string name, SymTabPtr ST = nullptr);

			FuncPtr getParent() { return Parent; }

			// getNext/Prev - Return the next or previous argument in the list.
			ArgPtr getNext() { return Next; }
			ArgPtr getPrev() { return Prev; }

			static bool classof(ValPtr V)
			{
				return V->getValueType() == Value::ValueTy::ArgumentVal;
			}
		};
	}
}

#endif
