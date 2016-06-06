//===--------------------------CodeGenMo
#include "../../include/IRBuild/IRBuilder.h"
using namespace compiler::IR;

void ModuleBuilder::VisitChildren(std::vector<std::shared_ptr<StatementAST>> AST)
{
	unsigned ASTSize = AST.size();
	for (int i = 0; i < ASTSize; i++)
	{
		// �ڱ���AST�Ĺ����У������AST�ľ���ڵ���ѡ���Ӧ��Accept������
		// ���磺��������� "IfStmt" �Ļ�����ѡ��IfStmt��Accept()����
		AST[i].get()->Accept(this);
	}
}