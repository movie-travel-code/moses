//===------------------------CodeGenTypes.cpp-----------------------------===//
//
// This file implements class CodeGenTypes (AST -> moses type lowering). 
//
//===---------------------------------------------------------------------===//
#include "../../include/IR/IRType.h"
#include "../../include/IRbuild/COdeGenTypes.h"
using namespace compiler::IRBuild;

std::shared_ptr<compiler::IR::Type> CodeGenTypes::ConvertType(std::shared_ptr<compiler::ast::Type> type)
{
	// (1) 检查相同的StructType是否生成过，生成过的话，直接返回。
	switch (type->getKind())
	{
	case ast::TypeKind::INT:
		return IRType::getIntType();
	case ast::TypeKind::BOOL:
		return IRType::getBoolType();
	case ast::TypeKind::USERDEFIED:
		return IRStructTy::Create(type);
		break;
	case ast::TypeKind::ANONYMOUS:
		return IRStructTy::get(type);
	case ast::TypeKind::VOID:
		return IRType::getVoidType();
	default:
		break;
	}
	return nullptr;
}
