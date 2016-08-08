//===--------------------------------main.cpp---------------------------------===//
// 
// 2016.3.7
// 
//===-------------------------------------------------------------------------===//
#include <iostream>
#include <sstream>
#include "../include/Lexer/scanner.h"
#include "../include/Parser/parser.h"
#include "../include/IRBuild/IRBuilder.h"
#include "../include//Parser/constant-evaluator.h"
// #include "../include/IR/MosesIRContext.h"
#include "../include/Parser/ASTContext.h"
#include "../include/IR/Support/IRPrinter.h"
#include "../include/ExecutionEngine/ExecutionEngine.h"

using namespace compiler::parse;
using namespace compiler::sema;
using namespace compiler::IR;
using namespace compiler::IRBuild;
using namespace compiler::Interpreter;
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

	// (4) IR write;
	std::ostringstream out;	
	IRPrinter::Print(IRContext, out);
	IRPrinter::Print(moduleBuilder.getIRs(), out);
	cout << out.str();

	std::ofstream mosesIR("E:/test/moses.mi");
	mosesIR << out.str();
	mosesIR.close();

	// (5) Interpreter
	auto interpreter = Interpreter::create(moduleBuilder.getIRs(), IRContext);
	interpreter->run();
	system("pause");
	return 0;
}