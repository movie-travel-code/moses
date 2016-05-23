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
#include "../Support/error.h"
#include "../Lexer/TokenKinds.h"

namespace compiler
{
	namespace ast
	{
		class FunctionDecl;
		// Note: VOID只用于函数返回类型
		enum class TypeKind : unsigned char
		{
			INT,
			BOOL,
			VOID,
			USERDEFIED, 
			ANONYMOUS
		};

		namespace TypeFingerPrint
		{
			// Why 此处如果不加const修饰符，会报重定义错误?
			// 虽然必须要加上const，因为TypeFingerPrint是不可修改的变量
			const std::string ConstFingerPrint = "0";
			const std::string IntFingerPrint = "1";
			const std::string BoolFingerPrint = "2";
			const std::string VoidFingerPrint = "3";
			/// 例如： 
			/// class info
			///	{
			///		var height : int;
			///		var male : bool;
			/// };
			////
			/// class person{
			///		var num : int;
			///		var mem : info;
			/// };
			/// 用户自定义类型person的finger print是需要记录info的结构类型信息的
			const std::string StructuralFingerPrint = "4";
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
			virtual std::string getTypeFingerPrint() const { return ""; };
			virtual std::string getTypeFingerPrintWithNoConst() const { return ""; };
			virtual ~Type() {}
		};

		class BuiltinType final : public Type
		{
		public:
			BuiltinType(TypeKind kind, bool isConst) : Type(kind, isConst) {}
			std::string getTypeFingerPrint() const override;
			std::string getTypeFingerPrintWithNoConst() const override;
		};

		/// \brief UserDefinedType - This Represents class type.
		// To Do: Shit Code!
		// 容易引起歧义.
		class UserDefinedType final : public Type
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

			bool HaveMember(std::string name) const;

			std::shared_ptr<Type> getMemberType(std::string name) const;

			virtual std::string getTypeFingerPrint() const override;
			virtual std::string getTypeFingerPrintWithNoConst() const override;

			virtual ~UserDefinedType() {}
		};		

		/// Note: AnonymousType是moses很重要的特性
		/// var num = {{132, 23}, num * 9, {false, 10}};
		/// 其中num所具有的类型就是匿名类型.
		/// Note: 当前匿名类型中不支持用户自定义类型。
		class AnonymousType final : public Type
		{
			AnonymousType() = delete;
			std::vector<std::shared_ptr<Type>> subTypes;
		public:
			AnonymousType(std::vector<std::shared_ptr<Type>> types) : 
				Type(TypeKind::ANONYMOUS, false), subTypes(types) {}
			virtual std::string getTypeFingerPrint() const override;
			virtual std::string getTypeFingerPrintWithNoConst() const override;
			std::shared_ptr<Type> getSubType(int index)
			{
				return subTypes[index];
			}
			unsigned getSubTypesNum() const { return subTypes.size(); };
			void getTypes(std::vector<std::shared_ptr<Type>>& types) const;
		};
	}
}
#endif