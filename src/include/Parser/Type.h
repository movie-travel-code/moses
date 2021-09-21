//===-------------------------------Type.h--------------------------------===//
//
// This file is used to define class Type.
//
//===---------------------------------------------------------------------===//
#ifndef TYPE_INCLUDE
#define TYPE_INCLUDE
#include "include/Lexer/TokenKinds.h"
#include "include/Support/Hasing.h"
#include "include/Support/TypeSet.h"
#include "include/Support/error.h"
#include <algorithm>
#include <cassert>
#include <memory>
#include <string>
#include <utility>
#include <vector>


namespace ast {
class FunctionDecl;
enum class TypeKind : unsigned char { INT, BOOL, VOID, USERDEFIED, ANONYMOUS };

class Type {
public:
  typedef std::shared_ptr<Type> TyPtr;

protected:
  TypeKind Kind;

public:
  Type(TypeKind kind, bool isConst) : Kind(kind) {}
  Type(TypeKind kind) : Kind(kind) {}

  virtual TyPtr const_remove() const;
  // Shit code!
  virtual std::size_t size() const { return 0; }
  virtual std::size_t MemberNum() const { return 1; }
  virtual std::pair<TyPtr, std::string> operator[](unsigned idx) const {
    assert(0 && "There is no chance to call this function.");
    return std::make_pair(nullptr, "");
  }
  virtual TyPtr StripOffShell() const { return nullptr; };

  bool operator==(const Type &rhs) const;
  TypeKind getKind() const { return Kind; }
  static TypeKind checkTypeKind(tok::TokenValue kind);

  virtual std::string getTypeName() const;
  virtual ~Type() {}
};

class BuiltinType final : public Type {
public:
  BuiltinType(TypeKind kind) : Type(kind) {}
  virtual unsigned long size() const {
    switch (Kind) {
    case ast::TypeKind::INT:
      return 32;
    case ast::TypeKind::BOOL:
      return 32;
    case ast::TypeKind::VOID:
      return 0;
    default:
      assert(0 && "Unreachable code!");
    }
    return 0;
  }
};

/// \brief UserDefinedType - This Represents class type.
// To Do: Shit Code!
class UserDefinedType final : public Type {
  // The user defined type, e.g. class A {}
  std::string TypeName;
  std::vector<std::pair<TyPtr, std::string>> subTypes;

public:
  UserDefinedType(TypeKind kind, const std::string &TypeName)
      : Type(kind), TypeName(TypeName) {}

  UserDefinedType(TypeKind kind, const std::string &TypeName,
                  std::vector<std::pair<TyPtr, std::string>> subTypes)
      : Type(kind), TypeName(TypeName) {}

  // StripOffShell
  // e.g.		class A
  //			{
  //				var m : int;
  //			};
  //			class B
  //			{
  //				var m : A;
  //			};
  TyPtr StripOffShell() const;

  bool HaveMember(const std::string &name) const;
  bool operator==(const Type &rhs) const;
  TyPtr getMemberType(const std::string &name) const;
  int getIdx(const std::string &name) const;
  std::size_t size() const;
  virtual std::size_t MemberNum() const { return subTypes.size(); }

  std::pair<TyPtr, std::string> operator[](unsigned index) const {
    return subTypes[index];
  }
  std::string getTypeName() const { return TypeName; }
  std::vector<std::pair<TyPtr, std::string>> getMemberTypes() const {
    return subTypes;
  }
  void addSubType(TyPtr subType, std::string name) {
    subTypes.push_back({subType, name});
  }

  virtual ~UserDefinedType() {}
};

class AnonymousType final : public Type {
  AnonymousType() = delete;
  std::vector<TyPtr> subTypes;

public:
  AnonymousType(std::vector<TyPtr> types)
      : Type(TypeKind::ANONYMOUS), subTypes(types) {}

  TyPtr getSubType(unsigned index) const {
    assert(index < subTypes.size() && "Index out of range!");
    return subTypes[index];
  }

  std::pair<TyPtr, std::string> operator[](unsigned idx) const {
    return std::make_pair(subTypes[idx], "");
  }

  std::vector<TyPtr> getSubTypes() const { return subTypes; }
  TyPtr StripOffShell() const;
  std::size_t getSubTypesNum() const { return subTypes.size(); };
  void getTypes(std::vector<TyPtr> &types) const;

  std::size_t size() const;
  virtual std::size_t MemberNum() const { return subTypes.size(); }
};

namespace TypeKeyInfo {
typedef std::shared_ptr<UserDefinedType> UDTyPtr;
typedef std::shared_ptr<ast::Type> TyPtr;
typedef std::shared_ptr<AnonymousType> AnonTyPtr;

using namespace Hashing;

struct UserDefinedTypeKeyInfo {
  struct KeyTy {
    std::vector<TyPtr> SubTypes;
    std::string Name;
    KeyTy(std::vector<TyPtr> SubTy, std::string Name)
        : SubTypes(SubTy), Name(Name) {}

    KeyTy(const UDTyPtr &U) : Name(U->getTypeName()) {
      for (auto item : U->getMemberTypes()) {
        SubTypes.push_back(item.first);
      }
    }

    bool operator==(const KeyTy &rhs) const {
      if (Name != rhs.Name)
        return false;
      if (SubTypes == rhs.SubTypes)
        return true;
      return false;
    }

    bool operator!=(const KeyTy &rhs) const { return !this->operator==(rhs); }
  };
  static unsigned long long getHashValue(const KeyTy &Key) {
    return hash_combine_range(hash_value(Key.Name), Key.SubTypes.begin(),
                              Key.SubTypes.end());
  }
  static unsigned long long getHashValue(const UDTyPtr &type) {
    return getHashValue(KeyTy(type));
  }
  static unsigned long long getAnonHashValue(const KeyTy &Key) {
    return hash_combine_range(0, Key.SubTypes.begin(), Key.SubTypes.end());
  }
  static unsigned long long getAnonHashValue(const UDTyPtr &type) {
    return getAnonHashValue(KeyTy(type));
  }
  static bool isEqual(const KeyTy &LHS, UDTyPtr RHS) {
    return LHS == KeyTy(RHS);
  }
  static bool isEqual(UDTyPtr LHS, UDTyPtr RHS) { return LHS == RHS; }
};

struct AnonTypeKeyInfo {
  struct KeyTy {
    std::vector<TyPtr> SubTypes;
    KeyTy(const std::vector<TyPtr> &E) : SubTypes(E) {}
    KeyTy(AnonTyPtr anony) : SubTypes(anony->getSubTypes()) {}

    bool operator==(const KeyTy &rhs) const {
      if (rhs.SubTypes == SubTypes)
        return true;
      return false;
    }
    bool operator!=(const KeyTy &rhs) const { return !this->operator==(rhs); }
  };
  static unsigned long long getHashValue(const KeyTy &Key) {
    return hash_combine_range(0, Key.SubTypes.begin(), Key.SubTypes.end());
  }
  static unsigned long long getHashValue(AnonTyPtr RHS) {
    return getHashValue(KeyTy(RHS));
  }
  static bool isEqual(const KeyTy &LHS, AnonTyPtr RHS) {
    return LHS == KeyTy(RHS);
  }
  static bool isEqual(AnonTyPtr LHS, AnonTyPtr RHS) { return LHS == RHS; }
};

struct TypeKeyInfo {
  static unsigned long long getHashValue(TyPtr type) {
    if (UDTyPtr UD = std::dynamic_pointer_cast<UserDefinedType>(type)) {
      return UserDefinedTypeKeyInfo::getHashValue(UD);
    }
    if (AnonTyPtr AnonT = std::dynamic_pointer_cast<AnonymousType>(type)) {
      return AnonTypeKeyInfo::getHashValue(AnonT);
    }
    if (std::shared_ptr<BuiltinType> BT =
            std::dynamic_pointer_cast<BuiltinType>(type)) {
      std::hash<std::shared_ptr<BuiltinType>> hasher;
      return hasher(BT);
    }
    return 0;
  }

  /// \brief This just for UserDefinedType.
  static unsigned long long getAnonHashValue(TyPtr type) {
    if (UDTyPtr UD = std::dynamic_pointer_cast<UserDefinedType>(type)) {
      return UserDefinedTypeKeyInfo::getHashValue(UD);
    }
    /// To Do: ErrorReport
    return 0;
  }
};
}; // namespace TypeKeyInfo
} // namespace ast
#endif