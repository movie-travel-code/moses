//===-----------------------------GenericValue.h--------------------------===//
//
// The GenericValue class is used to represent the Interpreter's value of
// arbitrary type.
//
//===---------------------------------------------------------------------===//
#pragma once

namespace Execution {
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
} // namespace Execution
