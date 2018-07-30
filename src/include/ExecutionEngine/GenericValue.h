//===-----------------------------GenericValue.h--------------------------===//
//
// The GenericValue class is used to represent the Interpreter's value of
// arbitrary type.
//
//===---------------------------------------------------------------------===//
#ifndef GENERIC_VALUE_H
#define GENERIC_VALUE_H
namespace compiler {
namespace Interpreter {
using PointerTy = void *;
struct GenericValue {
  union {
    bool BoolVal;
    int IntVal;
    PointerTy PointerVal;
  };
  GenericValue() {}
  explicit GenericValue(void *V) : PointerVal(V) {}
};
// PTOGV - PointerToGenericValue
inline GenericValue PTOGV(void *P) { return GenericValue(P); }
inline void *GVTOP(const GenericValue &GV) { return GV.PointerVal; }
} // namespace Interpreter
} // namespace compiler
#endif