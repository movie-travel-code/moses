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

unsigned Type::getSize() const
{
	switch (ID)
	{
	case compiler::IR::Type::VoidTy:
		return 0;
	case compiler::IR::Type::IntegerTy:
		return sizeof(int);
	case compiler::IR::Type::BoolTy:
		return sizeof(int);
	case compiler::IR::Type::PointerTy:
		return sizeof(void*);
	case compiler::IR::Type::LabelTy:
	case compiler::IR::Type::FunctionTy:
	case compiler::IR::Type::StructTy:
	case compiler::IR::Type::AnonyTy:
		break;
	default:
		break;
	}
}

/// \brief Print the Type info, i32 bool void and so on.
void Type::Print(std::ostringstream& out)
{
	switch (ID)
	{
	case compiler::IR::Type::VoidTy:
		out << " void";
		break;
	case compiler::IR::Type::LabelTy:
		out << " label";
		break;
	case compiler::IR::Type::IntegerTy:
		out << " int";
		break;
	case compiler::IR::Type::BoolTy:
		out << " bool";
		break;
	case compiler::IR::Type::FunctionTy:
		// oops. Have no idea to handle the funciton type's name.
		out << " function.type";
		break;
	default:
		break;
	}
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

/// \brief 如果函数没有参数的话。
std::shared_ptr<FunctionType> FunctionType::get(TyPtr retty)
{
	return std::make_shared<FunctionType>(retty);
}

/// 该函数用于获取param type.
/// 例如： returntype parm0 parm1 parm2
/// [0] = parm0
/// [2] = parm2
IRTyPtr FunctionType::operator[](unsigned index) const
{
	assert(index < NumContainedTys && 
		"Index out of range when we get parm IR type.");
	return ContainedTys[index];
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

/// \brief Print the function type info.
///	e.g.	int (*call)(int);
///			call = add;
///
///			%call = alloca i32 (i32)*					; <i32 (i32)**>
///					~~~~~~~~~~~~~~~~		--------> Funcition type <i32 (i32)>
///			store i32 (i32)* @add, i32 (i32)** %call
///					...
///			%1 = load i32 (i32)** %call					; <i32 (i32)*>
///					...
///			%4 = call i32 %2(i32 %3)
void FunctionType::Print(std::ostringstream& out)
{
	ContainedTys[0]->Print(out);
	out << " (";
	if (NumContainedTys > 1)
	{
		for (unsigned i = 0; i < NumContainedTys; i++)
		{
			ContainedTys[i]->Print(out);
			if (i != NumContainedTys - 1)
			{
				out << ",";
			}
		}
	}
	out << ")";
}

//===---------------------------------------------------------------------===//
// Implements class StructType.
/// 创建identified struct.

StructType::StructType(std::vector<IRTyPtr> members, std::string Name, bool isliteral) : 
	Type(TypeID::StructTy), Literal(isliteral), ContainedTys(members), 
	NumContainedTys(members.size()) 
{}

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
	std::string Name;
	if (ASTUDTyPtr UD = std::dynamic_pointer_cast<ASTUDTy>(type))
	{
		auto subtypes = UD->getMemberTypes();
		Name = UD->getTypeName();
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
	return std::make_shared<StructType>(members, Name, false);
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
	return std::make_shared<StructType>(members, "%anony.1", true);
}

/// \brief Print the struct type info.
/// e.g.	class Node
///			{
///				int value;
///				int neg;
///			};
///			%struct.Node = type { int, int }
///
///					or
///
///			var num = {int, bool};
///			%anony.1 = type {int, bool}
void StructType::PrintCompleteInfo(std::ostringstream& out)
{
	out << Name << " = struct.type {";
	if (NumContainedTys > 0)
	for (unsigned i = 0, size = ContainedTys.size(); i < size; i++)
	{
		ContainedTys[i]->Print(out);
		if (i == size - 1)
			break;
		out << ", ";
	}
		
	out << " }\n";
}

void StructType::Print(std::ostringstream& out)
{
	out << " " << Name;
}

void StructType::setName(std::string Name)
{
	this->Name = Name;
}

unsigned StructType::getSize() const
{
	unsigned sum = 0;
	for (auto item : ContainedTys)
	{
		sum += item->getSize();
	}
	return sum;
}

//===---------------------------------------------------------------------===//
// Implements the PointerType.
PointerType::PointerType(IRTyPtr Ty) : Type(TypeID::PointerTy)
{
	ElementTy = Ty;
}

IRPtTyPtr PointerType::get(IRTyPtr Ty)
{
	return std::make_shared<PointerType>(Ty);
}

/// \brief Print the pointer info.
/// e.g.	%retval = alloca i32			; <i32*>
void PointerType::Print(std::ostringstream& out)
{
	ElementTy->Print(out);
	out << "*";
}