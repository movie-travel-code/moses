//===----------------------------------IRType.h---------------------------===//
// 
// This file contains the declaration of the Type class.
//
//===---------------------------------------------------------------------===//
#ifndef MOSES_IR_IRTYPE_H
#define MOSES_IR_IRTYPE_H

namespace compiler
{
	namespace IR
	{
		/// The instance of the Type class are immutable: once thet are created,
		/// they are never changed. Also note that only one instance of a particular
		/// type is ever created. Thus seeing if two types are equal is a matter of
		/// doing a trivial pointer comparison.(但是moses有一种类型的转换的概念)
		/// To enforce that no two equal instances are created, Type instances can 
		/// only be created via static factory methods in class Type and in derived
		/// classes. Once allocated, Types are never free'd.
		class Type
		{
		public:
			//===----------------------------------------------------------===//
			// Definitions of all of the base types for the type system. Based
			// on this value, you can cast to a class defined in DerivedTypes.h.
			enum TypeID
			{
				VoidTyID,
				LabelTyID,
				IntegerTyID,
				BoolTyID,
				FunctionTyID,
				StructTyID,
				AnonyTyID
			};
		private:
			TypeID ID;
		protected:
		public:
			//===-----------------------------------------------------===//
			// Accessors for working with types.
			TypeID getTypeID() const { return ID; }

			bool isVoidType() const { return getTypeID() == VoidTyID; }

			bool isLabelTy() const { return getTypeID() == LabelTyID; }

			bool isIntegerTyID() const { return getTypeID() == IntegerTyID; }

			bool isFunctionTy() const { return getTypeID() == FunctionTyID; }

			bool isStructTy() const { return getTypeID() == StructTyID; }

			bool isAnonyTy() const { return getTypeID() == AnonyTyID; }

			bool isBoolTy() const { return getTypeID() == BoolTyID; }

			/// isSingleValueType - Return true if the type is a valid type for a 
			/// register in codegen. 
			bool isSingleValueType() const
			{
				return isIntegerTyID() || isBoolTy();
			}

			/// isAggregateType - Return true if the type is an aggregate type.
			bool isAggregateType() const
			{
				return getTypeID() == StructTyID || getTypeID() == AnonyTyID;
			}
		};
	}
}

#endif