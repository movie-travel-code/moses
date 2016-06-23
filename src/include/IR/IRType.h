//===----------------------------------IRType.h---------------------------===//
// 
// This file contains the declaration of the Type class.
// ����Type info��moses IR��ʹ��һ�����ݽṹContext������ȫ�ֵ�������Ϣ����moses��
// �û��Զ���������ȫ��Ψһ�ģ�����: class A{}, ��moses IR�о�ֻ��struct.type��
// ȫ��ֻ����һ�ݶ���������ͬʱ������ݶ�Type info.
//
//===---------------------------------------------------------------------===//
#ifndef MOSES_IR_IRTYPE_H
#define MOSES_IR_IRTYPE_H
#include <vector>
#include <memory>
#include <assert.h>
#include "../../include/Parser/Type.h"
namespace compiler
{
	namespace IR
	{		
		class Type;
		class StructType;
		class FunctionType;

		using ASTType = compiler::ast::Type;
		using ASTTyPtr = std::shared_ptr<ASTType>;
		using ASTBuiltinTy = compiler::ast::BuiltinType;
		using ASTUDTy = compiler::ast::UserDefinedType;
		using ASTUDTyPtr = std::shared_ptr<ASTUDTy>;
		using ASTTyKind = compiler::ast::TypeKind;
		using ASTAnonyTy = compiler::ast::AnonymousType;
		using IRTyPtr = std::shared_ptr<Type>;
		using IRStructTyPtr = std::shared_ptr<StructType>;
		using IRFuncTyPtr = std::shared_ptr<FunctionType>;
		/// \brief IR Type.
		class Type
		{
		public:
			//===----------------------------------------------------------===//
			// Definitions of all of the base types for the type system. Based
			// on this value, you can cast to a class defined in DerivedTypes.h.
			enum TypeID
			{
				VoidTy,
				LabelTy,
				IntegerTy,
				BoolTy,
				// ����Ķ��ж�Ӧ������
				FunctionTy,
				StructTy,
				PointerTy,
				AnonyTy
			};
		private:
			TypeID ID;
		public:
			Type(TypeID id) : ID(id) {}

			//===-----------------------------------------------------===//
			// Accessors for working with types.
			TypeID getTypeID() const { return ID; }

			bool isVoidType() const { return getTypeID() == VoidTy; }

			bool isLabelTy() const { return getTypeID() == LabelTy; }

			bool isIntegerTyID() const { return getTypeID() == IntegerTy; }

			bool isFunctionTy() const { return getTypeID() == FunctionTy; }

			bool isStructTy() const { return getTypeID() == StructTy; }

			bool isAnonyTy() const { return getTypeID() == AnonyTy; }

			bool isBoolTy() const { return getTypeID() == BoolTy; }

			bool isPointerTy() const { return getTypeID() == PointerTy; }

			/// isSingleValueType - Return true if the type is a valid type for a 
			/// register in codegen. 
			bool isSingleValueType() const
			{
				return isIntegerTyID() || isBoolTy();
			}

			/// isAggregateType - Return true if the type is an aggregate type.
			bool isAggregateType() const
			{
				return getTypeID() == StructTy || getTypeID() == AnonyTy;
			}

			//===------------------------------------------------------------===//
			// Helper for get types.
			static IRTyPtr getVoidType();
			static IRTyPtr getLabelType();
			static IRTyPtr getIntType();
			static IRTyPtr getBoolType();
		};

		/// \brief FunctionType - Class to represent function types.
		class FunctionType : public Type
		{
			// FunctionType��Ӧ�Ķ��󶼲��ǿɸ��Ƶġ�
			// Function��ǩ����moses����Ψһ�ġ�
			FunctionType(const FunctionType &) = delete;
			const FunctionType &operator=(const FunctionType &) = delete;

			FunctionType(ASTTyPtr Return, std::vector<ASTTyPtr> Params);
		private:
			/// ����FunctionType��˵��ContainedTys�����������β����͡�
			std::vector<IRTyPtr> ContainedTys;

			unsigned NumContainedTys;
		public:
			/// This static method is the primary way of constructing a FunctionType.
			static std::shared_ptr<FunctionType> get(ASTTyPtr Return, std::vector<ASTTyPtr> Params);

			/// Create a FunctionType taking no parameters.
			static std::shared_ptr<FunctionType> get(ASTTyPtr Return);

			IRTyPtr getReturnType() const { return ContainedTys[0]; }

			/// �ú������ڻ�ȡparam type.
			/// ���磺 returntype parm0 parm1 parm2
			/// [0] = parm0
			/// [2] = parm2
			IRTyPtr operator[](unsigned index) const;

			unsigned getNumParams() const { return NumContainedTys - 1; }

			std::vector<IRTyPtr> getParams() const{ return ContainedTys; }

			static bool classof(IRTyPtr Ty);
		private:
			std::vector<IRTyPtr> ConvertParmTypeToIRType(std::vector<ASTTyPtr> ParmTypes);
		};

		/// Class to represent struct types.
		/// ����moses IR��˵��������struct type:
		/// (1) Literal struct types (e.g { i32, i32 })
		/// (2) Identifier structs (e.g %foo)
		/// 
		/// ����Literal struct types��˵����Ӧmoses�е��������� "{int, {bool ,int }}"����������
		/// ��ʱ�򣬱�����������������Ϣ��
		/// 
		/// ����Identifier structs ��˵����Ӧmoses�е�class���͡�
		class StructType : public Type
		{
			StructType(const StructType &) = delete;
			const StructType &operator=(const StructType&) = delete;
		private:
			bool Literal;

			std::vector<IRTyPtr> ContainedTys;
			unsigned NumContainedTys;

			/// For a named struct that actually has a name, this is a pointer to the 
			/// symbol table entry for the struct. This is null if the type is an 
			/// literal struct or if it is a identified type that has an empty name.
			///
			/// To Do: ��sema�е�symbol����ʵʵ����IR������ϵ������
			/// void *SymbolTableEntry;			
		public:
			StructType(std::vector<IRTyPtr> members, bool isliteral);
			/// ����identified struct.
			static IRStructTyPtr Create(std::string Name);

			static IRStructTyPtr Create(std::vector<IRTyPtr> Elements,
				std::string Name);

			// ����AST�ϵ����ͣ�������StructType��
			static IRStructTyPtr Create(ASTTyPtr type);

			/// ����literal struct type.
			static IRStructTyPtr get(std::vector<IRTyPtr> Elements);
			/// ����literal struct type.
			static IRStructTyPtr get(ASTTyPtr type);

			bool isLiteral() const { return Literal; }

			/// Return the name for this struct type if it has an identity.
			/// This may return an empty string for an unnamed struct type. Do not call
			/// this on an literal type.
			std::string getName() const;

			/// Change the name of this type to the specified name.
			void setName(std::string Name);

			/// ������ߵĲ�����ͬ���򷵻�true.
			/// (�е��������ǰ���е�����ָ�Ƶĸ��ͨ������Ԫ�ؽ��м��㣬���ж��������Ƿ�����)
			bool isLayoutIdentical(IRStructTyPtr Other) const;

			unsigned getNumElements() const { return NumContainedTys; }

			std::vector<std::shared_ptr<Type>> getContainedTys() const { return ContainedTys; }

			IRTyPtr operator[](unsigned index) const
			{ 
				// �ж�index�Ƿ� inbound
				return ContainedTys[index]; 
			}

			static bool classof(IRTyPtr T)
			{
				return T->getTypeID() == StructTy;
			}
		};
	}
}

#endif