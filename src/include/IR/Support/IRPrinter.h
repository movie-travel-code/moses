//===------------------------------IRPrinter.h----------------------------===//
//
// This file implements the IRPrinter.
//
//===---------------------------------------------------------------------===//
#include "include/IR/IRType.h"
#include "include/IR/Instruction.h"
#include "include/IR/MosesIRContext.h"
#include "include/IR/User.h"
#include "include/IR/Value.h"
#include <iostream>
#include <vector>

namespace compiler {
namespace IR {
/// \brief The base class of BBPrinter and FunctionPrinter
class IRPrinter {
public:
  static void Print(const MosesIRContext &Ctx, std::ostringstream &out);
  static void Print(const std::list<ValPtr> &IR, std::ostringstream &out);
};
} // namespace IR
} // namespace compiler
