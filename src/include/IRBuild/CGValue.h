//===-------------------------------CGValue.h-----------------------------===//
//
// These classes implement wrappers around IR::value in order to fully
// represent the range of values for C L- and R- values.
//
//===---------------------------------------------------------------------===//
#pragma once
#include "IR/IRType.h"
#include "IR/Value.h"
#include "Parser/Type.h"
#include <cassert>

using namespace IR;
namespace CodeGen {
/// RValue - This trivial value class is used to represent the result of an
/// expression that is evaluated.
class RValue {
  enum { Scalar, Aggregate } Flavor;
  std::shared_ptr<Value> V1, V2;

public:
  bool isScalar() const { return Flavor == Scalar; }
  bool isAggregate() const { return Flavor == Aggregate; }

  std::shared_ptr<Value> getScalarVal() const { return V1; }

  /// getAggregateAddr() - Return the value of the address of the aggregate.
  std::shared_ptr<Value> getAggregateAddr() const { return V1; }

  static RValue get(std::shared_ptr<Value> V) {
    RValue ER;
    ER.V1 = V;
    ER.Flavor = Scalar;
    return ER;
  }

  static RValue getAggregate(std::shared_ptr<Value> V) {
    RValue ER;
    ER.V1 = V;
    ER.Flavor = Aggregate;
    return ER;
  }
};

class LValue {
  std::shared_ptr<Value> V;
  // address space.
  unsigned AddressSpace;

public:
  unsigned getAddressSpace() const { return AddressSpace; }
  std::shared_ptr<Value> getAddress() const { return V; }

  static LValue MakeAddr(std::shared_ptr<Value> Alloca, unsigned AddressSpace = 0) {
    LValue R;
    R.V = Alloca;
    R.AddressSpace = AddressSpace;
    return R;
  }
};
} // namespace CodeGen
