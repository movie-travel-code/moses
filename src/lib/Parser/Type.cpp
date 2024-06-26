//===--------------------------------Type.cpp-----------------------------===//
//
// This file is used to implement class Type.
//
//===---------------------------------------------------------------------===//
#include "Parser/Type.h"
using namespace ast;
using namespace tok;
//===---------------------------------------------------------------------===//
// Implements Type class.
TypeKind ASTType::checkTypeKind(TokenValue kind) {
  switch (kind) {
  case TokenValue::KEYWORD_int:
    return TypeKind::INT;
  case TokenValue::KEYWORD_bool:
    return TypeKind::BOOL;
  default:
    return TypeKind::USERDEFIED;
  }
}

std::string ASTType::getTypeName() const {
  switch (Kind) {
  case TypeKind::INT:
    return "int";
  case TypeKind::BOOL:
    return "bool";
  case TypeKind::VOID:
    return "void";
  case TypeKind::ANONYMOUS:
  case TypeKind::USERDEFIED:
    return "";
  }
  return "";
}

// remove const attribute.
std::shared_ptr<ASTType> ASTType::const_remove() const {
  return std::make_shared<ASTType>(Kind);
}

bool ASTType::operator==(const ASTType &rhs) const {
  if (Kind == rhs.getKind()) {
    return true;
  }
  return false;
}

//===---------------------------------------------------------------------===//
// Implement class UserDefinedType.
std::shared_ptr<ASTType>
UserDefinedType::getMemberType(const std::string &name) const {
  auto getType = [&]() -> std::shared_ptr<ASTType> {
    for (auto item : subTypes) {
      if (name == item.second) {
        return item.first;
      }
    }
    return nullptr;
  };
  return getType();
}

bool UserDefinedType::HaveMember(const std::string &name) const {
  for (auto item : subTypes) {
    if (item.second == name)
      return true;
  }
  return false;
}

int UserDefinedType::getIdx(const std::string &name) const {
  for (unsigned i = 0; i < subTypes.size(); i++) {
    if (subTypes[i].second == name)
      return i;
  }
  return -1;
}

unsigned long UserDefinedType::size() const {
  unsigned long size = 0;

  for (auto item : subTypes) {
    auto type = item.first;
    type->size();

    size += item.first->size();
  }
  return size;
}

std::shared_ptr<ASTType> UserDefinedType::StripOffShell() const {
  if (size() > 32)
    return nullptr;
  if (auto UDTy = std::dynamic_pointer_cast<UserDefinedType>(subTypes[0].first))
    return UDTy->StripOffShell();
  else
    return subTypes[0].first;
}

bool UserDefinedType::operator==(const ASTType &rhs) const {
  std::size_t subTypeNum = subTypes.size();
  try {
    const UserDefinedType &rhsUserDef =
        dynamic_cast<const UserDefinedType &>(rhs);
    for (unsigned i = 0; i < subTypeNum; i++) {
      if (*(subTypes[i].first) == *(rhsUserDef[i].first) &&
          subTypes[i].second == rhsUserDef[i].second) {
        return true;
      }
    }
  } catch (std::bad_cast b) {
    errorSema("Type incompatibility");
    return false;
  }
  return false;
}

//===---------------------------------------------------------------------===//
// Implement class AnonymousType.
void AnonymousType::getTypes(
    std::vector<std::shared_ptr<ASTType>> &types) const {
  std::size_t size = subTypes.size();
  for (unsigned index = 0; index < size; index++) {
    if (std::shared_ptr<AnonymousType> type =
            std::dynamic_pointer_cast<AnonymousType>(subTypes[index])) {
      type->getTypes(types);
    } else {
      types.push_back(subTypes[index]);
    }
  }
}

std::shared_ptr<ASTType> AnonymousType::StripOffShell() const {
  if (size() > 32)
    return nullptr;
  if (auto UDTy = std::dynamic_pointer_cast<AnonymousType>(subTypes[0]))
    return UDTy->StripOffShell();
  else
    return subTypes[0];
}

unsigned long AnonymousType::size() const {
  unsigned long size = 0;
  std::for_each(
      subTypes.begin(), subTypes.end(),
      [&size](const std::shared_ptr<ASTType> &ty) { size += ty->size(); });
  return size;
}
