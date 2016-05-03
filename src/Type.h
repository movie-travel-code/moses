//===-------------------------------Type.h--------------------------------===//
//
// This file is used to define class Type.
//
//===---------------------------------------------------------------------===//
#ifndef TYPE_INCLUDE
#define TYPE_INCLUDE
#include <string>
#include <vector>
#include <memory>
#include <utility>
#include "error.h"
#include "TokenKinds.h"

namespace compiler
{
	namespace ast
	{
		// Note: ast.h and Type.h refer to each other.
		class FunctionDecl;
		// Note: VOID只用于函数返回类型
		enum class TypeKind : unsigned char
		{
			INT,
			BOOL,
			VOID,
			USERDEFIED
		};

		class Type
		{
		protected:
			TypeKind Kind;
			bool IsConst;
		public:
			Type(TypeKind kind, bool isConst) : Kind(kind), IsConst(isConst){}
			bool operator==(const Type& rhs)
			{
				if (Kind == rhs.getKind() && IsConst == rhs.isConst())
				{
					return true;
				}
				return false;
			}
			bool isConst() const { return IsConst; }
			void setConst(bool isConst) { IsConst = isConst; }
			TypeKind getKind() const { return Kind; }
			static TypeKind checkTypeKind(tok::TokenValue kind);

			/// \brief 由于现在使用std::shared_ptr<Type>存储编译中的类型信息
			/// 由于重载的运算符只适用于Tpye对象，不适用于指针。
			/// 所以SB似的定义了一个TypeFingerPrint的概念来给编译中的类型定义一个唯一
			/// 的类型指纹，通过指纹来进行对比。同时获取指纹函数定义成virtual以便实现
			/// 多态。
			virtual std::string getTypeFingerPrint() const;
			virtual std::string getTypeFingerPrintWithNoConst() const;
			virtual ~Type() {}
		};

		class BuiltinType : public Type
		{
		public:
			BuiltinType(TypeKind kind, bool isConst) : Type(kind, isConst) {}
		};

		/// \brief UserDefinedType - This Represents class type.
		// To Do: Shit Code!
		// 容易引起歧义.
		class UserDefinedType : public Type
		{
			// The user defined type, e.g. class A {}
			std::string TypeName;
			std::vector< std::pair<std::shared_ptr<Type>, std::string> > subTypes;
		public:
			UserDefinedType(TypeKind kind, bool isConst, std::string TypeName) :
				Type(kind, isConst), TypeName(TypeName) {}

			bool operator==(const Type& rhs) const;

			std::pair<std::shared_ptr<Type>, std::string> operator[](int index) const
			{
				return subTypes[index];
			}

			std::string getTypeName() { return TypeName; }
			void addSubType(std::shared_ptr<Type> subType, std::string name) 
			{ 
				subTypes.push_back({ subType, name }); 
			}

			bool HaveMember(std::shared_ptr<Type> type, std::string name) const;

			std::shared_ptr<Type> getMemberType(std::string name) const;

			std::string getTypeFingerPrint() const override;
			std::string getTypeFingerPrintWithNoConst() const override;

			virtual ~UserDefinedType() {}
		};
	}
}
#endif