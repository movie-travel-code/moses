//===-------------------------------Type.h--------------------------------===//
//
// This file is used to define class Type.
//
//===---------------------------------------------------------------------===//
#ifndef TYPE_INCLUDE
#define TYPE_INCLUDE
#include <string>
#include <vector>
#include "ast.h"
#include "TokenKinds.h"

namespace compiler
{
	namespace ast
	{
		// Note: ast.h and Type.h refer to each other.
		class FunctionDecl;
		enum class TypeKind : unsigned char
		{
			INT,
			BOOL,
			USERDEFIED
		};

		class Type
		{
			TypeKind Kind;
			bool IsConst;
		public:
			Type(TypeKind kind, bool isConst) : Kind(kind), IsConst(isConst){}
			virtual bool isBuiltinType() = 0;
			bool isConst() { return IsConst; }
			TypeKind getKind() { return Kind; }
			static TypeKind checkTypeKind(tok::TokenValue kind);
			virtual ~Type() {}
		};

		class BuiltinType : public Type
		{
		public:
			BuiltinType(TypeKind kind, bool isConst) : Type(kind, isConst) {}
			// To Do
			// ����ͨ��inline����ʵ���������ͺ��Զ������͵��ж�
			// ���Ƕ�̬������ʱ�ģ���inline�Ǳ����ڵġ�virtual����������inline��
			// һ���򵥵Ĺ��ܻ���ͨ����������ʵ�֣����һ���ͨ�������ת
			bool isBuiltinType() { return true; }			
		};

		/// \brief UserDefinedType - This Represents class type.
		class UserDefinedType : Type
		{
			// The user defined type, e.g. class A {}
			std::string TypeName;
			// The statement which define this type.
			// method
			std::vector<FunctionDecl> method;
			// subtype
			std::vector<Type> subTypes;
		public:
			UserDefinedType(TypeKind kind, bool isConst, std::string TypeName) :
				Type(kind, isConst), TypeName(TypeName) {}
			bool isBuiltinType() { return false; }
			std::string getTypeName() { return TypeName; }
		};
	}
}
#endif