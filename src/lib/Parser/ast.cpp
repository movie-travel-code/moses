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
/// \brief ��Ҫ�������͵ļ�������
/// ���磺 var {num, {lhs, rhs}} �� type
/// To Do: ������������⣬ָ�뱩¶
bool UnpackDecl::TypeCheckingAndTypeSetting(AnonymousType* type)
{
	unsigned size = decls.size();
	// (1) ���unpack decl��size��Anonymous type����type�Ƿ���ͬ
	if (type->getSubTypesNum() != size)
	{
		return false;
	}

	// (2) �ݹ�������ÿ��element.
	// Note: ��������������ݽ�������type�����Բ�����type��������ָ��
	// ʹ��ͬһ��ԭ��ָ���ʼ����������ָ�룬�п��ܻ��������ָ�롣
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

/// \brief ��ȡdecl name
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

/// \brief ��ȡdecl names
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
	// (1) �������Ƿ�ֻ��һ�� return stmt
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