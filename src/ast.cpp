//===------------------------------ast.cpp--------------------------------===//
//
// This file is used to implement class stmt, decl.
// To Do: The semantic analyzer use syntax-directed pattern.
// 
//===---------------------------------------------------------------------===//
#include "ast.h"

namespace compiler
{
	namespace ast
	{
		void CompoundStmt::addSubStmt(StmtASTPtr stmt)
		{
			if (!stmt)
				SubStmts.push_back(std::move(stmt));
		}
	}
}