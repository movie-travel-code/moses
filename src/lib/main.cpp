//===--------------------------------main.cpp---------------------------------===//
//
// 2016.3.7
//
//===-------------------------------------------------------------------------===//
#include "Parser/constant-evaluator.h"
#include "ExecutionEngine/ExecutionEngine.h"
#include "IR/Dominators.h"
#include "IR/Support/IRPrinter.h"
#include "IRBuild/IRBuilder.h"
#include "Lexer/scanner.h"
#include "Parser/ASTContext.h"
#include "Parser/parser.h"
#include <iostream>
#include <sstream>


using namespace parse;
using namespace sema;
using namespace IR;
using namespace IRBuild;
using namespace Execution;
int main(int argc, char *argv[]) {
  // FIXME: We should use more mature approach to handle user options.
  if (argc != 2) {
    errorOption("Please provide one option to indicate the source code path.");
  }

  Scanner scanner(argv[1]);
  ASTContext Ctx;
  Sema sema(Ctx);
  Parser parse(scanner, sema, Ctx);
  // (1) parse and semantic analysis.
  parse.parse();

  // (2) check error.
  if (!Ctx.isParseOrSemaSuccess)
    exit(1);

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
  cout << "--------------------------------------------------------------------"
          "------------\n";

  // Set the output path.
  // FIXME: Maybe we should allow user to provide a output path.
  std::string OutputPath(argv[1]);
  OutputPath = OutputPath.substr(0, OutputPath.size() - 2);
  OutputPath += "mi";

  std::ofstream mosesIR(OutputPath);
  mosesIR << out.str();
  mosesIR.close();

  DominatorTree DomTree;
  std::vector<std::shared_ptr<BasicBlock>> CFG;
  for (auto item : moduleBuilder.getIRs()) {
    if (std::shared_ptr<BasicBlock> BB = std::dynamic_pointer_cast<BasicBlock>(item))
      CFG.push_back(BB);
  }
  DomTree.runOnCFG(CFG);

  DomTree.ComputeDomFrontierOnCFG(CFG);

  // (5) Interpreter
  auto interpreter = Interpreter::create(moduleBuilder.getIRs(), IRContext);
  interpreter->run();
  return 0;
}