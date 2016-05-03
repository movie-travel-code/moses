//===--------------------------------Type.cpp-----------------------------===//
// 
// This file is used to implement class Type.
// 
//===---------------------------------------------------------------------===//
#include "Type.h"

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

		std::string Type::getTypeFingerPrint() const
		{
			std::string fingerPrint;
			if (IsConst)
				fingerPrint = '0';
			else
				fingerPrint = '1';

			switch (Kind)
			{
			case compiler::ast::TypeKind::INT:
				fingerPrint += '1';
				break;
			case compiler::ast::TypeKind::BOOL:
				fingerPrint += '2';
				break;
			case compiler::ast::TypeKind::VOID:
				fingerPrint += '3';
				break;
			case compiler::ast::TypeKind::USERDEFIED:
				fingerPrint += '4';
				break;
			default:
				break;
			}
			return fingerPrint;
		}

		std::string Type::getTypeFingerPrintWithNoConst() const
		{
			std::string fingerPrint;

			switch (Kind)
			{
			case compiler::ast::TypeKind::INT:
				fingerPrint += '1';
				break;
			case compiler::ast::TypeKind::BOOL:
				fingerPrint += '2';
				break;
			case compiler::ast::TypeKind::VOID:
				fingerPrint += '3';
				break;
			case compiler::ast::TypeKind::USERDEFIED:
				fingerPrint += '4';
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
			};
			return getType();
		}

		bool UserDefinedType::HaveMember(std::shared_ptr<Type> type, std::string name) const
		{
			for (auto item : subTypes)
			{
				if (*(item.first) == *type && item.second == name)
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
					if (*(subTypes[i].first) == *(rhsUserDef[i].first) && subTypes[i].second == rhsUserDef[i].second)
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
			std::string fingerPrint;
			if (IsConst)
				fingerPrint = "0";
			else
				fingerPrint = "1";

			for (auto item : subTypes)
			{
				fingerPrint += item.first.get()->getTypeFingerPrint();
			}
			return fingerPrint;
		}

		std::string UserDefinedType::getTypeFingerPrintWithNoConst() const
		{
			std::string fingerPrint;
			if (IsConst)
				fingerPrint = "0";
			else
				fingerPrint = "1";

			for (auto item : subTypes)
			{
				fingerPrint += item.first.get()->getTypeFingerPrintWithNoConst();
			}
			return fingerPrint;
		}
	}
}