//===------------------------------ast.cpp--------------------------------===//
//
// This file is used to implement class stmt, decl.
// To Do: The semantic analyzer use syntax-directed pattern.
// 
//===---------------------------------------------------------------------===//
#include "../../include/Parser/ast.h"
using namespace compiler::ast;

//===----------------------CompoundStmt-------------------------===//
//
void CompoundStmt::addSubStmt(StmtASTPtr stmt)
{
	if (!stmt)
		SubStmts.push_back(std::move(stmt));
}

StmtASTPtr CompoundStmt::getSubStmt(unsigned index) const
{
	return SubStmts[index];
}

StmtASTPtr CompoundStmt::operator[](unsigned index) const
{
	assert(index < SubStmts.size() && "Index out of range!");
	return SubStmts[index];
}

//===---------------------------UnpackDecl-----------------------------===//
//
/// \brief 主要进行类型的检查和设置
/// 例如： var {num, {lhs, rhs}} 和 type
/// To Do: 代码设计有问题，指针暴露
bool UnpackDecl::TypeCheckingAndTypeSetting(AnonTyPtr type)
{
	unsigned size = decls.size();
	// (1) 检查unpack decl的size与Anonymous type的子type是否相同
	if (type->getSubTypesNum() != size)
	{
		return false;
	}

	// (2) 递归检查其中每个element.
	// Note: 由于这个函数传递进来的是type，所以不能以type构建智能指针
	// 使用同一个原生指针初始化两个智能指针，有可能会出现悬空指针。
	for (unsigned index = 0; index < size; index++)
	{
		if (UnpackDeclPtr unpackd = std::dynamic_pointer_cast<UnpackDecl>(decls[index]))
		{
			if (AnonTyPtr anonyt = std::dynamic_pointer_cast<AnonymousType>(type->getSubType(index)))
			{
				return unpackd->TypeCheckingAndTypeSetting(anonyt);
			}
			else
			{
				return false;
			}
		}
		else
		{
			if (AnonTyPtr anonyt = std::dynamic_pointer_cast<AnonymousType>(type->getSubType(index)))
			{
				return false;
			}
		}
	}
	return true;
}

/// \brief 获取decl name
std::vector<std::string> UnpackDecl::operator[](unsigned index) const
{
	std::vector<std::string> names;
	if (UnpackDeclPtr unpackd = std::dynamic_pointer_cast<UnpackDecl>(decls[index]))
	{
		unpackd->getDeclNames(names);
	}

	if (VarDeclPtr var = std::dynamic_pointer_cast<VarDecl>(decls[index]))
	{
		names.push_back(var->getName());
	}
	return names;
}

/// \brief 获取decl names
void UnpackDecl::getDeclNames(std::vector<std::string>& names) const
{
	unsigned size = decls.size();
	for (unsigned index = 0; index < size; index++)
	{
		if (UnpackDeclPtr unpackd = std::dynamic_pointer_cast<UnpackDecl>(decls[index]))
		{
			unpackd->getDeclNames(names);
		}

		if (VarDeclPtr var = std::dynamic_pointer_cast<VarDecl>(decls[index]))
		{
			names.push_back(var->getName());
		}
	}
}

void UnpackDecl::setCorrespondingType(std::shared_ptr<Type> type)
{
	declType = type;
}

//===--------------------------FunctionDecl--------------------------===//
//
ReturnStmtPtr FunctionDecl::isEvalCandiateAndGetReturnStmt() const
{
	if (returnType->getKind() != TypeKind::INT &&
		returnType->getKind() != TypeKind::BOOL)
	{
		return nullptr;
	}
	// (1) 函数体是否只有一条 return stmt
	if (CompoundStmtPtr body = std::dynamic_pointer_cast<CompoundStmt>(funcBody))
	{
		if (body->getSize() != 1)
			return false;
		if (ReturnStmtPtr returnStmt =
			std::dynamic_pointer_cast<ReturnStatement>(body->getSubStmt(0)))
		{
			return returnStmt;
		}
		return nullptr;
	}
	return nullptr;
}

bool FunctionDecl::endsWithReturn() const
{
	if (CompoundStmtPtr body = std::dynamic_pointer_cast<CompoundStmt>(funcBody))
	{
		// ends with return stmt.
		if (ReturnStmtPtr ret = std::dynamic_pointer_cast<ReturnStatement>((*body)[body->getSize() - 1]))
		{
			return true;
		}
	}
	return false;
}