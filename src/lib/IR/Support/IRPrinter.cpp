//===------------------------------IRPrinter.cpp--------------------------===//
//
// This file implements the functionality of class *Printer.
//
//===---------------------------------------------------------------------===//
#include "IR/Support/IRPrinter.h"
using namespace IR;

void IRPrinter::Print(const MosesIRContext &Ctx, std::ostringstream &out) {
  // (1) Print the anonymous structure type.
  for (const auto &item : Ctx.getAnonyTypes()) {
    item->PrintCompleteInfo(out);
    out << "\n";
  }

  // (2) Print the named structure type.
  for (const auto &item : Ctx.getNamedTypes()) {
    item->PrintCompleteInfo(out);
    out << "\n";
  }
}

void IRPrinter::Print(const std::list<ValPtr> &IR, std::ostringstream &out) {
  for (const auto &item : IR) {
    item->Print(out);
    out << "\n";
  }
}