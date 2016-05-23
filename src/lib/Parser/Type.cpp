//===--------------------------------Type.cpp-----------------------------===//
// 
// This file is used to implement class Type.
// 
//===---------------------------------------------------------------------===//
#include "../../include/Parser/Type.h"

namespace compiler
{
	namespace ast
	{
		TypeKind Type::checkTypeKind(tok::TokenValue kind)
		{
			switch (kind)
			{
			case compiler::tok::TokenValue::KEYWORD_int:
				return TypeKind::INT;
			case compiler::tok::TokenValue::KEYWORD_bool:
				return TypeKind::BOOL;
			default:
				return TypeKind::USERDEFIED;
			}
		}

		std::string BuiltinType::getTypeFingerPrint() const 
		{
			std::string fingerPrint;
			if (IsConst)
				fingerPrint = TypeFingerPrint::ConstFingerPrint;
			else
				fingerPrint = "";

			switch (Kind)
			{
			case compiler::ast::TypeKind::INT:
				fingerPrint += TypeFingerPrint::IntFingerPrint;
				break;
			case compiler::ast::TypeKind::BOOL:
				fingerPrint += TypeFingerPrint::BoolFingerPrint;
				break;
			case compiler::ast::TypeKind::VOID:
				fingerPrint += TypeFingerPrint::VoidFingerPrint;
				break;
			default:
				break;
			}
			return fingerPrint;
		}

		std::string BuiltinType::getTypeFingerPrintWithNoConst() const
		{
			std::string fingerPrint = "";

			switch (Kind)
			{
			case compiler::ast::TypeKind::INT:
				fingerPrint += TypeFingerPrint::IntFingerPrint;
				break;
			case compiler::ast::TypeKind::BOOL:
				fingerPrint += TypeFingerPrint::BoolFingerPrint;
				break;
			case compiler::ast::TypeKind::VOID:
				fingerPrint += TypeFingerPrint::VoidFingerPrint;
				break;
			default:
				break;
			}
			return fingerPrint;
		}

		std::shared_ptr<Type> UserDefinedType::getMemberType(std::string name) const
		{
			auto getType = [&]() -> std::shared_ptr<Type>
			{
				for (auto item : subTypes)
				{
					if (name == item.second)
					{
						return item.first;
					}
				}
				return nullptr;
			};
			return getType();
		}

		bool UserDefinedType::HaveMember(std::string name) const
		{
			for (auto item : subTypes)
			{
				if (item.second == name)
				{
					return true;
				}
			}
			return false;
		}

		bool UserDefinedType::operator==(const Type& rhs) const
		{
			int subTypeNum = subTypes.size();
			// 使用dynamic_cast<>对引用进行down_cast，转换失败会抛出bad_cast异常
			try
			{
				const UserDefinedType& rhsUserDef = dynamic_cast<const UserDefinedType&>(rhs);
				for (int i = 0; i < subTypeNum; i++)
				{
					if (*(subTypes[i].first) == *(rhsUserDef[i].first) && 
						subTypes[i].second == rhsUserDef[i].second)
					{
						return true;
					}
				}
			}
			catch (std::bad_cast b)
			{
				errorSema("Type incompatibility");
				return false;
			}
			return false;
		}

		std::string UserDefinedType::getTypeFingerPrint() const 
		{
			if (IsConst)
			{			
				return TypeFingerPrint::ConstFingerPrint + getTypeFingerPrintWithNoConst();
			}
			else
			{
				return getTypeFingerPrintWithNoConst();
			}
				
		}

		std::string UserDefinedType::getTypeFingerPrintWithNoConst() const
		{
			std::string fingerPrint = "";
			for (auto item : subTypes)
			{
				if (!(item.first.get()))
				{
					continue;
				}
				// (1) 记录结构信息，判断当前子类型是否为user defined type.
				if (UserDefinedType* udt = dynamic_cast<UserDefinedType*>(item.first.get()))
				{
					fingerPrint += TypeFingerPrint::StructuralFingerPrint;
				}
				// (2) Type fingerprint的拼接
				fingerPrint += item.first.get()->getTypeFingerPrint();
			}
			return fingerPrint;
		}

		/// Note: 对于匿名类型来说，{int, {int, int}} 和 {int, int, int}
		/// 是不同的，需要区别对待，所以记录结构信息
		std::string AnonymousType::getTypeFingerPrint() const
		{
			// AnonymousType默认是const的
			std::string fingerPrint = "";
			for (auto item : subTypes)
			{
				if (!(item.get()))
				{
					continue;
				}

				// (1) 需要记录结构类型信息
				// Note: AnonymousType是不允许存在user defined type的
				if (AnonymousType* anony = dynamic_cast<AnonymousType*>(item.get()))
				{
					fingerPrint += TypeFingerPrint::StructuralFingerPrint;
				}
				// (2) finger print的拼接
				fingerPrint += item.get()->getTypeFingerPrintWithNoConst();
			}
			return fingerPrint;
		}

		/// \brief AnonymousType类型的变量都是只读的，只能被解包。
		std::string AnonymousType::getTypeFingerPrintWithNoConst() const
		{
			return getTypeFingerPrint();
		}

		void AnonymousType::getTypes(std::vector<std::shared_ptr<Type>>& types) const
		{
			unsigned size = subTypes.size();
			for (int index = 0; index < size; index++)
			{
				if (AnonymousType* type = dynamic_cast<AnonymousType*>(subTypes[index].get()))
				{
					type->getTypes(types);
				}
				else
				{
					types.push_back(subTypes[index]);
				}
			}
		}
	}
}