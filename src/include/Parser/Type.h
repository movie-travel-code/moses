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
#include <cassert>
#include "../Support/error.h"
#include "../Lexer/TokenKinds.h"
#include "../Support/Hasing.h"
#include "../Support/TypeSet.h"

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

		class Type
		{
		public:
			typedef std::shared_ptr<Type> TyPtr;
		protected:
			TypeKind Kind;
		public:
			Type(TypeKind kind, bool isConst) : Kind(kind){}
			Type(TypeKind kind) : Kind(kind) {}

			virtual TyPtr const_remove() const;

			bool operator==(const Type& rhs) const;
 			TypeKind getKind() const { return Kind; }
			static TypeKind checkTypeKind(tok::TokenValue kind);

			virtual ~Type() {}
		};

		class BuiltinType final : public Type
		{
		public:
			BuiltinType(TypeKind kind) : Type(kind) {}
		};

		/// \brief UserDefinedType - This Represents class type.
		// To Do: Shit Code!
		// 容易引起歧义.
		class UserDefinedType final : public Type
		{
			// The user defined type, e.g. class A {}
			std::string TypeName;
			std::vector< std::pair<TyPtr, std::string> > subTypes;
		public:
			UserDefinedType(TypeKind kind, std::string TypeName) :
				Type(kind), TypeName(TypeName) {}

			UserDefinedType(TypeKind kind, std::string TypeName,
				std::vector<std::pair<TyPtr, std::string>> subTypes) :
				Type(kind), TypeName(TypeName) {}
					
			void addSubType(TyPtr subType, std::string name) { subTypes.push_back({ subType, name }); }

			bool HaveMember(std::string name) const;
			bool operator==(const Type& rhs) const;
			std::pair<TyPtr, std::string> operator[](unsigned index) const { return subTypes[index]; }
			std::string getTypeName() { return TypeName; }
			TyPtr getMemberType(std::string name) const;
			std::vector<std::pair<TyPtr, std::string>> getMemberTypes() const;

			virtual ~UserDefinedType() {}
		};		

		/// Note: AnonymousType是moses很重要的特性
		/// var num = {{132, 23}, num * 9, {false, 10}};
		/// 其中num所具有的类型就是匿名类型.
		/// Note: 当前匿名类型中不支持用户自定义类型。
		class AnonymousType final : public Type
		{
			AnonymousType() = delete;
			std::vector<TyPtr> subTypes;
		public:
			AnonymousType(std::vector<TyPtr> types) : 
				Type(TypeKind::ANONYMOUS), subTypes(types) {}

			TyPtr getSubType(unsigned index) const;
			std::vector<TyPtr> getSubTypes() const;

			unsigned getSubTypesNum() const { return subTypes.size(); };
			void getTypes(std::vector<TyPtr>& types) const;
		};

		namespace TypeKeyInfo
		{
			typedef std::shared_ptr<UserDefinedType> UDTyPtr;
			typedef std::shared_ptr<ast::Type> TyPtr;
			typedef std::shared_ptr<AnonymousType> AnonTyPtr;

			using namespace Hashing;			

			struct UserDefinedTypeKeyInfo
			{
				struct KeyTy
				{
					std::vector<TyPtr> SubTypes;
					std::string Name;
					KeyTy(std::vector<TyPtr> SubTy, std::string Name) : SubTypes(SubTy), Name(Name) {}

					KeyTy(const UDTyPtr& U) : Name(U->getTypeName())
					{
						for (auto item : U->getMemberTypes())
						{
							SubTypes.push_back(item.first);
						}						
					}

					bool operator==(const KeyTy& rhs) const
					{
						if (Name != rhs.Name)
							return false;
						if (SubTypes == rhs.SubTypes)
							return true;
						return false;
					}

					bool operator!=(const KeyTy& rhs) const { return !this->operator==(rhs); }
				};
				static unsigned long long getHashValue(const KeyTy& Key)
				{
					return hash_combine_range(hash_value(Key.Name), Key.SubTypes.begin(),
						Key.SubTypes.end());
				}
				static unsigned long long getHashValue(const UDTyPtr& type) { return getHashValue(KeyTy(type)); }
				static unsigned long long getAnonHashValue(const KeyTy& Key)
				{
					return hash_combine_range(0, Key.SubTypes.begin(), Key.SubTypes.end());
				}
				static unsigned long long getAnonHashValue(const UDTyPtr& type)
				{
					return getAnonHashValue(KeyTy(type));
				}
				static bool isEqual(const KeyTy& LHS, UDTyPtr RHS) { return LHS == KeyTy(RHS); }
				static bool isEqual(UDTyPtr LHS, UDTyPtr RHS) { return LHS == RHS; }
			};

			struct AnonTypeKeyInfo
			{
				struct KeyTy
				{
					std::vector<TyPtr> SubTypes;
					KeyTy(const std::vector<TyPtr>& E) : SubTypes(E) {}
					KeyTy(AnonTyPtr anony) : SubTypes(anony->getSubTypes()) {}

					bool operator==(const KeyTy& rhs) const
					{
						if (rhs.SubTypes == SubTypes)
							return true;
						return false;
					}
					bool operator!=(const KeyTy& rhs) const
					{
						return !this->operator==(rhs);
					}
				};
				static unsigned long long getHashValue(const KeyTy& Key)
				{
					return hash_combine_range(0, Key.SubTypes.begin(), Key.SubTypes.end());
				}
				static unsigned long long getHashValue(AnonTyPtr RHS) { return getHashValue(KeyTy(RHS)); }
				static bool isEqual(const KeyTy& LHS, AnonTyPtr RHS) { return LHS == KeyTy(RHS); }
				static bool isEqual(AnonTyPtr LHS, AnonTyPtr RHS) { return LHS == RHS; }

			};

			struct TypeKeyInfo
			{
				static unsigned long long getHashValue(TyPtr type)
				{
					if (UDTyPtr UD = std::dynamic_pointer_cast<UserDefinedType>(type))
					{
						return UserDefinedTypeKeyInfo::getHashValue(UD);
					}

					if (AnonTyPtr AnonT = std::dynamic_pointer_cast<AnonymousType>(type))
					{
						return AnonTypeKeyInfo::getHashValue(AnonT);
					}
				}

				/// \brief This just for UserDefinedType.
				static unsigned long long getAnonHashValue(TyPtr type)
				{
					if (UDTyPtr UD = std::dynamic_pointer_cast<UserDefinedType>(type))
					{
						return UserDefinedTypeKeyInfo::getHashValue(UD);
					}
					else
					{
						/// To Do: ErrorReport
						return 0;
					}
				}
			};
		};
	}
}
#endif