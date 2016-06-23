//===------------------------------User.h---------------------------------===//
//
// This class defines the interface that one who uses a Value must implement.
// Each instance of the Value class keeps track of what User's have handles
// to it.
//
// * Instructions are the largest class of Users.
// * Constants may be users of other constants 
//
//===---------------------------------------------------------------------===//

#ifndef MOSES_IR_USR_H
#define MOSES_IR_USR_H
#include <vector>
#include <string>
#include "Value.h"

namespace compiler
{
	namespace IR
	{
		/// User - The User class provides a basis for expressing the ownership of User
		/// towards other Value instances.
		class User : public Value
		{
			User(const User&) = delete;
		protected:
			std::vector<UsePtr> Operands;

			/// Allocate a User with an operand pointer co-allocated.
			///
			/// This is used for subclasses which need to allocate a variable number
			/// of operands, ie, 'hung off uses'.
			// void *operator new(size_t Size);

			/// allocate a User with the operands co-allocated.
			///
			/// This is used for subclasses which have a fixed number of operands.
			// void *operator new(size_t Size, unsigned Us);
		public:
			User(TyPtr Ty, ValueTy vty, const std::string &name = "") : Value(Ty, vty, name)
			{

			}

			UsePtr getOperand(unsigned i)
			{
				// Check whether out of range.
				return Operands[i];
			}

			void setOeprand(unsigned i, ValPtr Val)
			{
				// Check whether out of range.
				// Operands[i] = Val;
			}

			unsigned getNumOperands() const { return Operands.size(); }

			// dropAllReferences() - This function is in charge of "letting go" of all
			// objects that this User refers to. This allows one to 'delete' a whole
			// class at a time, even though there may be circular references... first 
			// all references are dropped, and all use counts go to zero. Then everything
			// is delete'd for real. Note that no operations are valid on an object that 
			// has "dropped all references", except operator delete.
			void dropAllReferences()
			{
				Operands.clear();
			}

			/// replaceUsesofWith - Replaces all references to the "From" definition with
			/// references to the "To" definition.
			void replaceUsesOfWith(ValPtr From, ValPtr To);

			// Methods for support type inquiry through isa, case, and dyn_cast:
			/*static bool classof(ValPtr V)
			{				
			}*/
		};
	}
}

#endif