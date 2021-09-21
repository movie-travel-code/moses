//===------------------Hashing.h - Utilities for hashing------------------===//
//
// This file implements the hash_combine() method.
// http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2012/n3333.html
// C++11 supply hash<> functions, but doesn't support hash_combine.
// http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n3876.pdf
// and
// http://stackoverflow.com/questions/2590677/how-do-i-combine-hash-values-in-c0x
//
//===---------------------------------------------------------------------===//
#pragma once
#include <functional>
#include <string>
#include <type_traits>
#include <utility>

namespace Hashing {
template <typename T> void hash_combine(size_t &seed, const T &val) {
  std::hash<T> hasher;
  seed ^= hasher(val) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

// Auxiliary generic functions to create a hash value using a seed.
template <typename T, typename... Types>
void hash_combine(size_t &seed, const T &val, const Types &... args) {
  hash_combine(seed, val);
  hash_combine(seed, args...);
}

// Optional auxiliary generic functions to support hash_val() without
// arguments
// void hash_combine(size_t& seed) {}

// Generic function to create a hash value out of a heterogeneous list
// of arguments.
template <typename... Types> size_t hash_value(const Types &... args) {
  size_t seed = 0;
  hash_combine(seed, args...);
  return seed;
}

// Generic function to create a hash value for list of arguments.
template <typename ValueT>
size_t hash_combine_range(size_t seed, ValueT begin, ValueT end) {
  for (; begin != end; begin++) {
    hash_combine(seed, *begin);
  }
  return seed;
}
} // namespace Hashing
