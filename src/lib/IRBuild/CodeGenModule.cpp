//===--------------------------CodeGenMo
#include "../../include/IRBuild/IRBuilder.h"
using namespace compiler::IR;

void ModuleBuilder::VisitChildren(std::vector<std::shared_ptr<StatementAST>> AST)
{
	unsigned ASTSize = AST.size();
	for (int i = 0; i < ASTSize; i++)
	{
		// 在遍历AST的过程中，会根据AST的具体节点来选择对应的Accept函数。
		// 例如：如果这里是 "IfStmt" 的话，会选择IfStmt的Accept()函数
		AST[i].get()->Accept(this);
	}
}