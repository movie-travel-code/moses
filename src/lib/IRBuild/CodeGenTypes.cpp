//===------------------------CodeGenTypes.cpp-----------------------------===//
//
// This file implements class CodeGenTypes (AST -> moses type lowering).
//
//===---------------------------------------------------------------------===//
#include "../../include/IRbuild/CodeGenTypes.h"
#include "../../include/IR/IRType.h"

using namespace compiler::IRBuild;

std::shared_ptr<compiler::IR::Type>
CodeGenTypes::ConvertType(std::shared_ptr<compiler::ast::Type> type) {
  assert(type && "CovertType must be non-null!");
  std::shared_ptr<StructType> IRType = nullptr;
  // (1) Check type cache.
  auto iter = RecordDeclTypes.find(type.get());
  if (iter != RecordDeclTypes.end())
    IRType = iter->second;

  // (2) Create new type.
  ast::TypeKind tyKind = type->getKind();
  assert((tyKind == ast::TypeKind::INT || tyKind == ast::TypeKind::BOOL ||
          tyKind == ast::TypeKind::VOID ||
          tyKind == ast::TypeKind::USERDEFIED ||
          tyKind == ast::TypeKind::ANONYMOUS) &&
         "TypeKind not exists.");

  if (tyKind == ast::TypeKind::INT) {
    return IRCtx.getIntTy();
  }
  if (tyKind == ast::TypeKind::BOOL) {
    return IRCtx.getBoolTy();
  }
  if (tyKind == ast::TypeKind::VOID) {
    return IRCtx.getVoidTy();
  }
  if (tyKind == ast::TypeKind::USERDEFIED) {
    // Create new named structure type.
    IRType = IRStructTy::Create(IRCtx, type);
    IRType->setName(TypeNamePrefix + "struct." + type->getTypeName());
    // Cached this type into the RecordTypes.
    RecordDeclTypes.insert({type.get(), IRType});
    // Storing this info.
    IRCtx.AddNamedStructType(IRType);
  }
  if (tyKind == ast::TypeKind::ANONYMOUS) {
    // Created new literal structure type.
    IRType = IRStructTy::get(IRCtx, type);
    IRType->setName(getAnonyName());
    // Cached this type into the RecordTypes.
    RecordDeclTypes.insert({type.get(), IRType});
    IRCtx.AddStructType(IRType);
  }
  return IRType;
}

std::string CodeGenTypes::getAnonyName() {
  return TypeNamePrefix + "anony." + std::to_string(AnonyTypesCounter++);
}