//===------------------------------IRPrinter.cpp--------------------------===//
//
// This file implements the functionality of class *Printer.
//
//===---------------------------------------------------------------------===//
#include "../../../include/IR/Support/IRPrinter.h"
using namespace compiler::IR;

void IRPrinter::Print(const std::list<ValPtr>& IR, std::ostringstream& out)
{
	for (auto item : IR)
	{
		item->Print(out);
	}
}