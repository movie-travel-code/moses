//===--------------------------------IRType.cpp---------------------------===//
//
// This file implements type class.
//
//===---------------------------------------------------------------------===//
#include "../../include/IR/IRType.h"
using namespace compiler::IR;
using TyPtr = std::shared_ptr<Type>;
using StructTyPtr = std::shared_ptr<StructType>;

//===---------------------------------------------------------------------===//
// Implements class Type.
IRTyPtr Type::getVoidType()
{
	return std::make_shared<Type>(TypeID::VoidTy);
}

IRTyPtr Type::getLabelType()
{
	return std::make_shared<Type>(TypeID::LabelTy);
}

IRTyPtr Type::getIntType()
{
	return std::make_shared<Type>(TypeID::IntegerTy);
}

IRTyPtr Type::getBoolType()
{
	return std::make_shared<Type>(TypeID::BoolTy);
}

//===---------------------------------------------------------------------===//
// Implements class FunctionType.

FunctionType::FunctionType(TyPtr retty, std::vector<TyPtr> parmsty) : Type(TypeID::FunctionTy)
{
	ContainedTys.push_back(retty);
	for (auto item : parmsty)
	{
		ContainedTys.push_back(item);
	}
	NumContainedTys = ContainedTys.size();
}

FunctionType::FunctionType(TyPtr retty) : Type(TypeID::FunctionTy)
{
	ContainedTys.push_back(retty);
	NumContainedTys = 1;
}

std::shared_ptr<FunctionType> FunctionType::get(TyPtr retty, std::vector<TyPtr> parmsty)
{
	return std::make_shared<FunctionType>(retty, parmsty);
}

/// \brief �������û�в����Ļ���
std::shared_ptr<FunctionType> FunctionType::get(TyPtr retty)
{
	return std::make_shared<FunctionType>(retty);
}

/// �ú������ڻ�ȡparam type.
/// ���磺 returntype parm0 parm1 parm2
/// [0] = parm0
/// [2] = parm2
IRTyPtr FunctionType::operator[](unsigned index) const
{
	assert(index != 0 && index <= NumContainedTys - 1 && 
		"Index out of range when we get parm IR type.");
	return ContainedTys[index];
}

bool FunctionType::classof(IRTyPtr Ty)
{
	return Ty->getTypeID() == FunctionTy;
}

std::vector<IRTyPtr> FunctionType::ConvertParmTypeToIRType(std::vector<ASTTyPtr> ParmTyps)
{
	// Note:����û�н��д�����Ҳ������if elseif else��else ��û�б���
	// ��Ϊ�ܹ����е���һ����˵��û���﷨���������.
	std::vector<IRTyPtr> IRTypes;
	// (1) ����parm type vector����ת����
	for (auto item : ParmTyps)
	{
		switch (item->getKind())
		{
		case ASTTyKind::INT:
			IRTypes.push_back(Type::getIntType());
			break;
		case ASTTyKind::BOOL:
			IRTypes.push_back(Type::getBoolType());
			break;
		case ASTTyKind::USERDEFIED:
			break;
		case ASTTyKind::ANONYMOUS:
			break;
		case ASTTyKind::VOID:
			break;
		default:
			break;
		}		
	}
	// (2) ��ת�����type���ء�
	return IRTypes;
}

//===---------------------------------------------------------------------===//
// Implements class StructType.
/// ����identified struct.

StructType::StructType(std::vector<IRTyPtr> members, bool isliteral) : 
	Type(TypeID::StructTy), Literal(isliteral), ContainedTys(members), 
	NumContainedTys(members.size()) 
{}

std::shared_ptr<StructType> StructType::Create(std::string Name)
{
	return nullptr;
}

std::shared_ptr<StructType> StructType::Create(std::vector<IRTyPtr> Elements,
	std::string Name)
{
	return nullptr;
}

/// \brief ����ָ����AST class ����������llvm�е�������Ϣ��
/// ���磺
///		class B
///		{
///			var num : int;
///			var flag : bool;
///			var a : A;
///		}
std::shared_ptr<StructType> StructType::Create(ASTTyPtr type)
{
	std::vector<IRTyPtr> members;
	if (ASTUDTyPtr UD = std::dynamic_pointer_cast<ASTUDTy>(type))
	{
		auto subtypes = UD->getMemberTypes();
		// ����һ���ݹ����.
		for (auto item : subtypes)
		{
			switch (item.first->getKind())
			{
			case ASTTyKind::BOOL:
				members.push_back(Type::getBoolType());
				break;
			case ASTTyKind::INT:
				members.push_back(Type::getIntType());
				break;
			case ASTTyKind::USERDEFIED:
				members.push_back(StructType::Create(item.first));
				break;
			default:
				break;
			}
		}
	}
	return std::make_shared<StructType>(members, false);
}

/// ����literal struct type.
std::shared_ptr<StructType> StructType::get(std::vector<IRTyPtr> Elements)
{
	return nullptr;
}

std::shared_ptr<StructType> StructType::get(ASTTyPtr type)
{
	std::vector<IRTyPtr> members;
	if (ASTUDTyPtr UD = std::dynamic_pointer_cast<ASTUDTy>(type))
	{
		auto subtypes = UD->getMemberTypes();
		// ����һ���ݹ����.
		for (auto item : subtypes)
		{
			switch (item.first->getKind())
			{
			case ASTTyKind::BOOL:
				members.push_back(Type::getBoolType());
				break;
			case ASTTyKind::INT:
				members.push_back(Type::getIntType());
				break;
			case ASTTyKind::USERDEFIED:
				members.push_back(StructType::Create(item.first));
				break;
			case ASTTyKind::ANONYMOUS:
				members.push_back(StructType::get(item.first));
				break;
			default:
				break;
			}
		}
	}
	return std::make_shared<StructType>(members, true);
}