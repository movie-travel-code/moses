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

const StatementAST* CompoundStmt::getSubStmt(unsigned index) const
{
	return SubStmts[index].get();
}

const StatementAST* CompoundStmt::operator[](unsigned index) const
{
	return SubStmts[index].get();
}

//===---------------------------UnpackDecl-----------------------------===//
//
/// \brief 主要进行类型的检查和设置
/// 例如： var {num, {lhs, rhs}} 和 type
/// To Do: 代码设计有问题，指针暴露
bool UnpackDecl::TypeCheckingAndTypeSetting(AnonymousType* type)
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
	for (int index = 0; index < size; index++)
	{
		if (UnpackDecl* unpackd = dynamic_cast<UnpackDecl*>(decls[index].get()))
		{
			if (AnonymousType* anonyt = dynamic_cast<AnonymousType*>(type->getSubType(index).get()))
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
			if (AnonymousType* anonyt = dynamic_cast<AnonymousType*>(type->getSubType(index).get()))
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
	if (UnpackDecl* unpackd = dynamic_cast<UnpackDecl*>(decls[index].get()))
	{
		unpackd->getDeclNames(names);
	}

	if (VarDecl* var = dynamic_cast<VarDecl*>(decls[index].get()))
	{
		names.push_back(var->getName());
	}
	return names;
}

/// \brief 获取decl names
void UnpackDecl::getDeclNames(std::vector<std::string>& names) const
{
	unsigned size = decls.size();
	for (int index = 0; index < size; index++)
	{
		if (UnpackDecl* unpackd = dynamic_cast<UnpackDecl*>(decls[index].get()))
		{
			unpackd->getDeclNames(names);
		}

		if (VarDecl* var = dynamic_cast<VarDecl*>(decls[index].get()))
		{
			names.push_back(var->getName());
		}
	}
}

void UnpackDecl::setCorrespondingType(std::shared_ptr<Type> type)
{
	this->type = type;
}

//===--------------------------FunctionDecl--------------------------===//
//
const StatementAST* FunctionDecl::isEvalCandiateAndGetReturnStmt() const
{
	if (returnType->getKind() != TypeKind::INT ||
		returnType->getKind() != TypeKind::BOOL)
	{
		return nullptr;
	}
	// (1) 函数体是否只有一条 return stmt
	if (const CompoundStmt* body = dynamic_cast<CompoundStmt*>(funcBody.get()))
	{
		if (body->getSize() != 1)
			return false;
		if (const ReturnStatement* returnStmt =
			dynamic_cast<const ReturnStatement*>(body->getSubStmt(0)))
		{
			return body;
		}
		return nullptr;
	}
	return nullptr;
}