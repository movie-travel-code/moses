//===------------------------------IRPrinter.cpp--------------------------===//
//
// This file implements the functionality of class *Printer.
//
//===---------------------------------------------------------------------===//
#include "include/IR/Support/IRPrinter.h"
using namespace compiler::IR;

void IRPrinter::Print(const MosesIRContext &Ctx, std::ostringstream &out) {
  // (1) Print the anonymous structure type.
  for (auto item : Ctx.getAnonyTypes()) {
    item->PrintCompleteInfo(out);
    out << "\n";
  }

  // (2) Print the named structure type.
  for (auto item : Ctx.getNamedTypes()) {
    item->PrintCompleteInfo(out);
    out << "\n";
  }
}

void IRPrinter::Print(const std::list<ValPtr> &IR, std::ostringstream &out) {
  for (auto item : IR) {
    item->Print(out);
    out << "\n";
  }
}