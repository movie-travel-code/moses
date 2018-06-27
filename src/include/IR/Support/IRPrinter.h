//===------------------------------IRPrinter.h----------------------------===//
//
// This file implements the IRPrinter.
//
//===---------------------------------------------------------------------===//
#include "../IRType.h"
#include "../Instruction.h"
#include "../MosesIRContext.h"
#include "../User.h"
#include "../Value.h"
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
