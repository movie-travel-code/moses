//===-------------------------------Type.h--------------------------------===//
//
// This file is used to define class Type.
//
//===---------------------------------------------------------------------===//
#pragma once
#include "Lexer/TokenKinds.h"
#include "Support/Hasing.h"
#include "Support/TypeSet.h"
#include "Support/error.h"
#include <algorithm>
#include <cassert>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace ast {
class FunctionDecl;
enum class TypeKind : unsigned char { INT, BOOL, VOID, USERDEFIED, ANONYMOUS };

class ASTType {
public:
protected:
  TypeKind Kind;

public:
  ASTType(TypeKind kind, [[maybe_unused]] bool isConst) : Kind(kind) {}
  ASTType(TypeKind kind) : Kind(kind) {}

  virtual std::shared_ptr<ASTType> const_remove() const;
  // Shit code!
  virtual std::size_t size() const { return 0; }
  virtual std::size_t MemberNum() const { return 1; }
  virtual std::pair<std::shared_ptr<ASTType>, std::string>
  operator[]([[maybe_unused]] unsigned idx) const {
    assert(0 && "There is no chance to call this function.");
    return std::make_pair(nullptr, "");
  }
  virtual std::shared_ptr<ASTType> StripOffShell() const { return nullptr; };

  bool operator==(const ASTType &rhs) const;
  TypeKind getKind() const { return Kind; }
  static TypeKind checkTypeKind(tok::TokenValue kind);

  virtual std::string getTypeName() const;
  virtual ~ASTType() {}
};

class BuiltinType final : public ASTType {
public:
  BuiltinType(TypeKind kind) : ASTType(kind) {}
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
class UserDefinedType final : public ASTType {
  // The user defined type, e.g. class A {}
  std::string TypeName;
  std::vector<std::pair<std::shared_ptr<ASTType>, std::string>> subTypes;

public:
  UserDefinedType(TypeKind kind, const std::string &TypeName)
      : ASTType(kind), TypeName(TypeName) {}

  UserDefinedType(TypeKind kind, const std::string &TypeName,
                  [[maybe_unused]] std::vector<
                      std::pair<std::shared_ptr<ASTType>, std::string>>
                      subTypes)
      : ASTType(kind), TypeName(TypeName) {}

  // StripOffShell
  // e.g.		class A
  //			{
  //				var m : int;
  //			};
  //			class B
  //			{
  //				var m : A;
  //			};
  std::shared_ptr<ASTType> StripOffShell() const;

  bool HaveMember(const std::string &name) const;
  bool operator==(const ASTType &rhs) const;
  std::shared_ptr<ASTType> getMemberType(const std::string &name) const;
  int getIdx(const std::string &name) const;
  std::size_t size() const;
  virtual std::size_t MemberNum() const { return subTypes.size(); }

  std::pair<std::shared_ptr<ASTType>, std::string>
  operator[](unsigned index) const {
    return subTypes[index];
  }
  std::string getTypeName() const { return TypeName; }
  std::vector<std::pair<std::shared_ptr<ASTType>, std::string>>
  getMemberTypes() const {
    return subTypes;
  }
  void addSubType(std::shared_ptr<ASTType> subType, std::string name) {
    subTypes.push_back({subType, name});
  }

  virtual ~UserDefinedType() {}
};

class AnonymousType final : public ASTType {
  AnonymousType() = delete;
  std::vector<std::shared_ptr<ASTType>> subTypes;

public:
  AnonymousType(std::vector<std::shared_ptr<ASTType>> types)
      : ASTType(TypeKind::ANONYMOUS), subTypes(types) {}

  std::shared_ptr<ASTType> getSubType(unsigned index) const {
    assert(index < subTypes.size() && "Index out of range!");
    return subTypes[index];
  }

  std::pair<std::shared_ptr<ASTType>, std::string>
  operator[](unsigned idx) const {
    return std::make_pair(subTypes[idx], "");
  }

  std::vector<std::shared_ptr<ASTType>> getSubTypes() const { return subTypes; }
  std::shared_ptr<ASTType> StripOffShell() const;
  std::size_t getSubTypesNum() const { return subTypes.size(); };
  void getTypes(std::vector<std::shared_ptr<ASTType>> &types) const;

  std::size_t size() const;
  virtual std::size_t MemberNum() const { return subTypes.size(); }
};

namespace TypeKeyInfo {
typedef std::shared_ptr<UserDefinedType> UDTyPtr;
typedef std::shared_ptr<AnonymousType> AnonTyPtr;

using namespace Hashing;

struct UserDefinedTypeKeyInfo {
  struct KeyTy {
    std::vector<std::shared_ptr<ASTType>> SubTypes;
    std::string Name;
    KeyTy(std::vector<std::shared_ptr<ASTType>> SubTy, std::string Name)
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
    std::vector<std::shared_ptr<ASTType>> SubTypes;
    KeyTy(const std::vector<std::shared_ptr<ASTType>> &E) : SubTypes(E) {}
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
  static unsigned long long getHashValue(std::shared_ptr<ASTType> type) {
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
  static unsigned long long getAnonHashValue(std::shared_ptr<ASTType> type) {
    if (UDTyPtr UD = std::dynamic_pointer_cast<UserDefinedType>(type)) {
      return UserDefinedTypeKeyInfo::getHashValue(UD);
    }
    /// To Do: ErrorReport
    return 0;
  }
};
}; // namespace TypeKeyInfo
} // namespace ast
