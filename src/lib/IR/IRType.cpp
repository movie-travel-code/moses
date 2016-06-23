//===--------------------------------IRType.cpp---------------------------===//
//
// This file implements type class.
//
//===---------------------------------------------------------------------===//
#include "../../include/IR/IRType.h"
using namespace compiler::IR;
// using IRTyPtr = std::shared_ptr<Type>;
// using IRStructTyPtr = std::shared_ptr<StructType>;

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

/// \brief 该函数用于产生FunctionType。
/// 例如：
///		class A { var num : int; var flag : bool; };
///		func add(lhs:int, rhs:int) -> A
///		{
///			return {lhs, true};
///		}
///	Note: 为function add创建FunctionType的时候，首先需要为return type和parameter type
/// 创建IR:Type。
std::shared_ptr<FunctionType> FunctionType::get(ASTTyPtr Return, std::vector<ASTTyPtr> Params)
{
	// (1) 首先将函数形参类型转化为moses IR类型.

	// (2) 然后将返回类型转换为moses IR类型
	// (3) 最后构造得到FunctionType
	return nullptr;
}

/// \brief 如果函数没有参数的话。
std::shared_ptr<FunctionType> FunctionType::get(ASTTyPtr Return)
{
	return nullptr;
}

/// 该函数用于获取param type.
/// 例如： returntype parm0 parm1 parm2
/// [0] = parm0
/// [2] = parm2
IRTyPtr FunctionType::operator[](unsigned index) const
{
	if (index == 0)
	{
		// operator[] 只能用于取得
	}
	if (index > NumContainedTys - 2)
	{
		// 报错
	}
	return ContainedTys[index + 1];
}

bool FunctionType::classof(IRTyPtr Ty)
{
	return Ty->getTypeID() == FunctionTy;
}

std::vector<IRTyPtr> FunctionType::ConvertParmTypeToIRType(std::vector<ASTTyPtr> ParmTyps)
{
	// Note:这里没有进行错误处理（也就是在if elseif else，else 中没有报错）
	// 因为能够进行到这一步的说明没有语法和语义错误.
	std::vector<IRTyPtr> IRTypes;
	// (1) 遍历parm type vector依次转换。
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
	// (2) 将转换后的type返回。
	return IRTypes;
}

//===---------------------------------------------------------------------===//
// Implements class StructType.
/// 创建identified struct.

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

/// \brief 根据指定的AST class 类型来创建llvm中的类型信息。
/// 例如：
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
		// 这是一个递归过程.
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

/// 创建literal struct type.
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
		// 这是一个递归过程.
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