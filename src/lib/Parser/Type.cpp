//===--------------------------------Type.cpp-----------------------------===//
// 
// This file is used to implement class Type.
// 
//===---------------------------------------------------------------------===//
#include "../../include/Parser/Type.h"
using namespace compiler::ast;
using namespace compiler::tok;
typedef std::shared_ptr<Type> TyPtr;

//===---------------------------------------------------------------------===//
// Implements Type class.
TypeKind Type::checkTypeKind(TokenValue kind)
{
	switch (kind)
	{
	case TokenValue::KEYWORD_int:
		return TypeKind::INT;
	case TokenValue::KEYWORD_bool:
		return TypeKind::BOOL;
	default:
		return TypeKind::USERDEFIED;
	}
}

// remove const attribute.
TyPtr Type::const_remove() const 
{
	return std::make_shared<Type>(Kind);
}

bool Type::operator == (const Type& rhs) const
{
	if (Kind == rhs.getKind())
	{
		return true;
	}
	return false;
}

//===---------------------------------------------------------------------===//
// Implement class UserDefinedType.

/// \brief ����Member Name��ȡ������������Ϣ
std::shared_ptr<Type> UserDefinedType::getMemberType(std::string name) const
{
	auto getType = [&]() -> TyPtr
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
			return true;
	}
	return false;
}

unsigned long UserDefinedType::size() const
{
	unsigned long size = 0;
	for (auto item : subTypes)
	{
		size += item.first->size();
	}
	return size;
}

bool UserDefinedType::operator==(const Type& rhs) const
{
	unsigned subTypeNum = subTypes.size();
	// ʹ��dynamic_cast<>�����ý���down_cast��ת��ʧ�ܻ��׳�bad_cast�쳣
	try
	{
		const UserDefinedType& rhsUserDef = dynamic_cast<const UserDefinedType&>(rhs);
		for (unsigned i = 0; i < subTypeNum; i++)
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

//===---------------------------------------------------------------------===//
// Implement class AnonymousType.

/// Note: ��������������˵��{int, {int, int}} �� {int, int, int}
/// �ǲ�ͬ�ģ���Ҫ����Դ������Լ�¼�ṹ��Ϣ
void AnonymousType::getTypes(std::vector<std::shared_ptr<Type>>& types) const
{
	unsigned size = subTypes.size();
	for (unsigned index = 0; index < size; index++)
	{
		if (std::shared_ptr<AnonymousType> type = std::dynamic_pointer_cast<AnonymousType>(subTypes[index]))
		{
			type->getTypes(types);
		}
		else
		{
			types.push_back(subTypes[index]);
		}
	}
}

unsigned long AnonymousType::size() const 
{
	unsigned long size = 0;
	std::for_each(subTypes.begin(), subTypes.end(), 
		[&size](const TyPtr& ty){ size += ty->size(); });
	return size;
}