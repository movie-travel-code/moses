//===-------------------------------CGValue.h-----------------------------===//
//
// These classes implement wrappers around IR::value in order to fully
// represent the range of values for C L- and R- values.
//
//===---------------------------------------------------------------------===//
#ifndef CG_VALUE_H
#define CG_VALUE_H
#include "include/IR/IRType.h"
#include "include/IR/Value.h"
#include "include/Parser/Type.h"
#include <cassert>

namespace compiler {
namespace CodeGen {
/// RValue - This trivial value class is used to represent the result of an
/// expression that is evaluated.
class RValue {
  enum { Scalar, Aggregate } Flavor;
  IR::ValPtr V1, V2;

public:
  bool isScalar() const { return Flavor == Scalar; }
  bool isAggregate() const { return Flavor == Aggregate; }

  IR::ValPtr getScalarVal() const { return V1; }

  /// getAggregateAddr() - Return the value of the address of the aggregate.
  IR::ValPtr getAggregateAddr() const { return V1; }

  static RValue get(IR::ValPtr V) {
    RValue ER;
    ER.V1 = V;
    ER.Flavor = Scalar;
    return ER;
  }

  static RValue getAggregate(IR::ValPtr V) {
    RValue ER;
    ER.V1 = V;
    ER.Flavor = Aggregate;
    return ER;
  }
};

class LValue {
  IR::ValPtr V;
  // address space.
  unsigned AddressSpace;

public:
  unsigned getAddressSpace() const { return AddressSpace; }
  IR::ValPtr getAddress() const { return V; }

  static LValue MakeAddr(IR::ValPtr Alloca, unsigned AddressSpace = 0) {
    LValue R;
    R.V = Alloca;
    R.AddressSpace = AddressSpace;
    return R;
  }
};
} // namespace CodeGen
} // namespace compiler

#endif