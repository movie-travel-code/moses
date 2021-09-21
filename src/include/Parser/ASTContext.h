//===------------------------------ASTContext.h---------------------------===//
//
// This file defines the ASTContext interface.
//
//===---------------------------------------------------------------------===//
#pragma once
#include "Support/Hasing.h"
#include "Support/TypeSet.h"
#include "Type.h"

using namespace SupportStructure;
namespace ast {
/// ASTContext - This class holds types that can be referred to thorought
/// the semantic analysis of a file.
class ASTContext {
private:
  using UDKeyInfo = TypeKeyInfo::UserDefinedTypeKeyInfo;
  using AnonTypeKeyInfo = TypeKeyInfo::AnonTypeKeyInfo;

public:
  ASTContext()
      : Int(std::make_shared<BuiltinType>(TypeKind::INT)),
        Bool(std::make_shared<BuiltinType>(TypeKind::BOOL)),
        Void(std::make_shared<BuiltinType>(TypeKind::VOID)),
        isParseOrSemaSuccess(true) {}

  std::shared_ptr<BuiltinType> Int;
  std::shared_ptr<BuiltinType> Bool;
  std::shared_ptr<BuiltinType> Void;

  TypeSet<std::shared_ptr<UserDefinedType>, UDKeyInfo> UDTypes;
  TypeSet<std::shared_ptr<AnonymousType>, AnonTypeKeyInfo> AnonTypes;

  bool isParseOrSemaSuccess;
};
} // namespace ast
