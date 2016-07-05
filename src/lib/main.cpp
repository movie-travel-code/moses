//===--------------------------------main.cpp---------------------------------===//
// 
// 2016.3.7
// 
//===-------------------------------------------------------------------------===//
#include <iostream>
#include "../include/Lexer/scanner.h"
#include "../include/Parser/parser.h"
#include "../include/IRBuild/IRBuilder.h"
#include "../include//Parser/constant-evaluator.h"
// ------ include header file for testing------------------//
#include "../include/IR/MosesIRContext.h"
#include "../include/Parser/ASTContext.h"
// ------ include header file for testing------------------//
using namespace compiler::parse;
using namespace compiler::sema;
using namespace compiler::IR;
using namespace compiler::IRBuild;
int main()
{
	Scanner scanner("E:/test/main.mo");
	ASTContext Ctx;
	Sema sema(Ctx);	
	Parser parse(scanner, sema, Ctx);
	// (1) parse and semantic analysis.
	parse.parse();

	// (2) check error.
	if (!Ctx.isParseOrSemaSuccess)
	{
		system("pause");
		return 0;		
	}

	// ConstantEvaluator evaluator;
	// ≤‚ ‘evaluator
	auto AST = parse.getAST();
	MosesIRContext IRContext;
	ModuleBuilder moduleBuilder(sema.getScopeStackBottom(), IRContext);
	// (3) IR Build.
	moduleBuilder.VisitChildren(parse.getAST());
	system("pause");
	return 0;
}