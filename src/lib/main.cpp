//===--------------------------------main.cpp---------------------------------===//
//
// 2016.3.7
//
//===-------------------------------------------------------------------------===//
#include "include/Parser/constant-evaluator.h"
#include "include/ExecutionEngine/ExecutionEngine.h"
#include "include/IR/Dominators.h"
#include "include/IR/Support/IRPrinter.h"
#include "include/IRBuild/IRBuilder.h"
#include "include/Lexer/scanner.h"
#include "include/Parser/ASTContext.h"
#include "include/Parser/parser.h"
#include <iostream>
#include <sstream>


using namespace compiler::parse;
using namespace compiler::sema;
using namespace compiler::IR;
using namespace compiler::IRBuild;
using namespace compiler::Interpreter;
int main() {
  Scanner scanner("/Users/wangliushuai/workspace/opensource/moses/test/main.mo");
  ASTContext Ctx;
  Sema sema(Ctx);
  Parser parse(scanner, sema, Ctx);
  // (1) parse and semantic analysis.
  parse.parse();

  // (2) check error.
  if (!Ctx.isParseOrSemaSuccess) {
    system("pause");
    return 0;
  }

  // ConstantEvaluator evaluator;
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

  std::ofstream mosesIR("/Users/wangliushuai/workspace/opensource/moses/test/main.mi");
  mosesIR << out.str();
  mosesIR.close();

  //=============================test=================================
  DominatorTree DomTree;
  std::vector<BBPtr> CFG;
  for (auto item : moduleBuilder.getIRs()) {
    if (BBPtr BB = std::dynamic_pointer_cast<BasicBlock>(item))
      CFG.push_back(BB);
  }
  DomTree.runOnCFG(CFG);

  DomTree.ComputeDomFrontierOnCFG(CFG);
  //=================================================================

  // (5) Interpreter
  auto interpreter = Interpreter::create(moduleBuilder.getIRs(), IRContext);
  interpreter->run();
  system("pause");
  return 0;
}