//===-------------------------------TypeSet.h-----------------------------===//
//
// This file defines the TypeSet class.
//
//===---------------------------------------------------------------------===//
#ifndef TYPE_SET_H
#define TYPE_SET_H
#include <vector>

namespace SupportStructure {
template <typename ValueT, typename ValueInfoT> class TypeSet {
  std::vector<ValueT> Buckets;

public:
  using key_type = ValueT;
  using value_type = ValueT;

  explicit TypeSet(unsigned NumInit = 0) {}

  bool empty() const { return Buckets.empty(); }

  std::size_t size() const { return Buckets.size(); }

  bool isIn(const ValueT &V) const {
    if (lookup(V))
      return true;
    return false;
  }

  std::vector<ValueT> getBuckets() const { return Buckets; }

  ValueT lookup(const ValueT &V) const {
    for (auto item : Buckets) {
      // auto item_hash = ValueInfoT::getHashValue(item);
      // auto v_hash = ValueInfoT::getHashValue(item);
      if (ValueInfoT::getHashValue(item) == ValueInfoT::getHashValue(item))
        return item;
    }
    return nullptr;
  }

  void insert(ValueT V) {
    if (lookup(V))
      return;
    Buckets.push_back(V);
  }
};
} // namespace SupportStructure
#endif