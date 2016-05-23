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
		// Note: VOIDֻ���ں�����������
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
			// Why �˴��������const���η����ᱨ�ض������?
			// ��Ȼ����Ҫ����const����ΪTypeFingerPrint�ǲ����޸ĵı���
			const std::string ConstFingerPrint = "0";
			const std::string IntFingerPrint = "1";
			const std::string BoolFingerPrint = "2";
			const std::string VoidFingerPrint = "3";
			/// ���磺 
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
			/// �û��Զ�������person��finger print����Ҫ��¼info�Ľṹ������Ϣ��
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

			/// \brief ��������ʹ��std::shared_ptr<Type>�洢�����е�������Ϣ
			/// �������ص������ֻ������Tpye���󣬲�������ָ�롣
			/// ����SB�ƵĶ�����һ��TypeFingerPrint�ĸ������������е����Ͷ���һ��Ψһ
			/// ������ָ�ƣ�ͨ��ָ�������жԱȡ�ͬʱ��ȡָ�ƺ��������virtual�Ա�ʵ��
			/// ��̬��
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
		// ������������.
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

		/// Note: AnonymousType��moses����Ҫ������
		/// var num = {{132, 23}, num * 9, {false, 10}};
		/// ����num�����е����;�����������.
		/// Note: ��ǰ���������в�֧���û��Զ������͡�
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