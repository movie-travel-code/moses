//===------------------------------IRPrinter.h----------------------------===//
//
// This file implements the IRPrinter.
//
//===---------------------------------------------------------------------===//
#include "IR/IRType.h"
#include "IR/Instruction.h"
#include "IR/MosesIRContext.h"
#include "IR/User.h"
#include "IR/Value.h"
#include <iostream>
#include <vector>

namespace IR {
/// \brief The base class of BBPrinter and FunctionPrinter
class IRPrinter {
public:
  static void Print(const MosesIRContext &Ctx, std::ostringstream &out);
  static void Print(const std::list<ValPtr> &IR, std::ostringstream &out);
};
} // namespace IR
