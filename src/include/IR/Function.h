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
			void setParent(FuncPtr parent);
		public:
			/// Argument ctor - If Function argument is specified, this argument is inserted at
			/// the end of the argument list for the function.
			Argument(TyPtr Ty, std::string Name = "", FuncPtr F = nullptr);
			void setType(TyPtr ty) { this->Ty = Ty; }
			FuncPtr getParent() { return Parent; }
			static bool classof(ValPtr V)
			{
				return V->getValueType() == Value::ValueTy::ArgumentVal;
			}
		};

		class Function : public GlobalValue
		{
		private:
			// Important things that make up a function!
			std::list<BBPtr> BasicBlocks;
			std::vector<ArgPtr> Arguments;
			SymTabPtr SymTab;
		public:
			Function(FuncTypePtr Ty, std::string Name = "") :
				GlobalValue(Ty, Value::ValueTy::FunctionVal, Name)
			{
				// Create space for argument and set the name later.
				for (unsigned i = 0; i < Ty->getNumParams(); i++)
				{
					Arguments.push_back(std::make_shared<Argument>((*Ty)[i + 1]));
				}
			}
			~Function() {}

			static FuncPtr create(FuncTypePtr Ty, std::string Name);

			ArgPtr operator[](unsigned index) const
			{
				assert(index <= Arguments.size() - 1 && 
					"Index out of range when we get the specified Argument(IR).");
				return Arguments[index];
			}

			/// \brief Set arguemtn name and type.
			void setArgumentInfo(unsigned index, std::string name)
			{
				assert(index <= Arguments.size() - 1 && 
					"Index out of range when set Argument(IR) name.");
				Arguments[index]->setName(name);
			}

			ArgPtr getArg(unsigned index) const { return (*this)[index]; }

			TyPtr getReturnType() const;
			TyPtr getFunctionType() const { return Ty; }

			/// getIntrinsicID - This method returns the ID number of the specified function.
			/// This value is always defined to be zero to allow easy checking for whether
			// a function if intrinsic or not. The paticular intrinsic functions which
			/// correspond to this value are defined in Intrinsics.h
			unsigned getIntrinsicID() const;
			bool isIntrinsic() const { return getIntrinsicID() != 0; }

			/// Get the underlying elements of the Function... the basic block list is empty
			/// for external functions.
			std::vector<ArgPtr> &getArgumentList() { return Arguments; }

			std::list<BBPtr> &getBasicBlockList() { return BasicBlocks; }

			ArgPtr operator[](unsigned index) { return Arguments[index]; }

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
	}
}

#endif
