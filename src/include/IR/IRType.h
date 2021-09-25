//===----------------------------------IRType.h---------------------------===//
//
// This file contains the declaration of the Type class.
//
//===---------------------------------------------------------------------===//
#pragma once
#include "Parser/Type.h"
#include <cassert>
#include <iostream>
#include <memory>
#include <sstream>
#include <vector>

namespace IR {
class Type;
class StructType;
class FunctionType;
class PointerType;
class MosesIRContext;

using ASTType = ast::Type;
using ASTTyPtr = std::shared_ptr<ASTType>;
using ASTBuiltinTy = ast::BuiltinType;
using ASTUDTy = ast::UserDefinedType;
using ASTUDTyPtr = std::shared_ptr<ASTUDTy>;
using ASTTyKind = ast::TypeKind;
using ASTAnonyTy = ast::AnonymousType;
using IRTyPtr = std::shared_ptr<Type>;
using IRStructTyPtr = std::shared_ptr<StructType>;
using IRFuncTyPtr = std::shared_ptr<FunctionType>;
using IRPtTyPtr = std::shared_ptr<PointerType>;
/// \brief IR Type.
class Type {
public:
  //===----------------------------------------------------------===//
  // Definitions of all of the base types for the type system. Based
  // on this value, you can cast to a class defined in DerivedTypes.h.
  enum class TypeID {
    VoidTy,
    LabelTy,
    IntegerTy,
    BoolTy,
    FunctionTy,
    StructTy,
    PointerTy,
    AnonyTy
  };

private:
  TypeID ID;

public:
  Type(TypeID id) : ID(id) {}
  virtual ~Type() {}
  //===-----------------------------------------------------===//
  // Accessors for working with types.
  TypeID getTypeID() const { return ID; }
  bool isVoidType() const { return getTypeID() == TypeID::VoidTy; }
  bool isLabelTy() const { return getTypeID() == TypeID::LabelTy; }
  bool isIntegerTy() const { return getTypeID() == TypeID::IntegerTy; }
  bool isFunctionTy() const { return getTypeID() == TypeID::FunctionTy; }
  bool isStructTy() const { return getTypeID() == TypeID::StructTy; }
  bool isAnonyTy() const { return getTypeID() == TypeID::AnonyTy; }
  bool isBoolTy() const { return getTypeID() == TypeID::BoolTy; }
  bool isPointerTy() const { return getTypeID() == TypeID::PointerTy; }

  /// isSingleValueType - Return true if the type is a valid type for a
  /// register in codegen.
  bool isSingleValueType() const { return isIntegerTy() || isBoolTy(); }

  /// isAggregateType - Return true if the type is an aggregate type.
  bool isAggregateType() const {
    return getTypeID() == TypeID::StructTy || getTypeID() == TypeID::AnonyTy;
  }
  //===------------------------------------------------------------===//
  // Helper for get types.
  static IRTyPtr getVoidType(MosesIRContext &Ctx);
  static IRTyPtr getLabelType(MosesIRContext &Ctx);
  static IRTyPtr getIntType(MosesIRContext &Ctx);
  static IRTyPtr getBoolType(MosesIRContext &Ctx);

  virtual unsigned getSize() const;

  /// \brief Print the type info.
  virtual void Print(std::ostringstream &out);
};

/// \brief FunctionType - Class to represent function types.
class FunctionType : public Type {
  FunctionType(const FunctionType &) = delete;
  const FunctionType &operator=(const FunctionType &) = delete;

private:
  std::vector<IRTyPtr> ContainedTys;

  unsigned NumContainedTys;

public:
  FunctionType(IRTyPtr retty, std::vector<IRTyPtr> parmsty);
  FunctionType(IRTyPtr retty);
  /// This static method is the primary way of constructing a FunctionType.
  static std::shared_ptr<FunctionType> get(IRTyPtr retty,
                                           std::vector<IRTyPtr> parmtys);

  /// Create a FunctionType taking no parameters.
  static std::shared_ptr<FunctionType> get(IRTyPtr retty);

  IRTyPtr getReturnType() const { return ContainedTys[0]; }

  /// param type.
  /// returntype parm0 parm1 parm2
  /// [0] = parm0
  /// [2] = parm2
  IRTyPtr operator[](unsigned index) const;

  unsigned getNumParams() const { return NumContainedTys - 1; }
  std::vector<IRTyPtr> getParams() const { return ContainedTys; }

  static bool classof(IRTyPtr Ty);

  /// \brief Print the FunctionType info.
  void Print(std::ostringstream &out) override;

private:
  std::vector<IRTyPtr> ConvertParmTypeToIRType(MosesIRContext &Ctx,
                                               std::vector<ASTTyPtr> ParmTypes);
};

/// Class to represent struct types.
/// struct type:
/// (1) Literal struct types (e.g { i32, i32 })
/// (2) Identifier structs (e.g %foo)
class StructType : public Type {
  StructType(const StructType &) = delete;
  const StructType &operator=(const StructType &) = delete;

private:
  bool Literal;
  std::string Name;
  std::vector<IRTyPtr> ContainedTys;
  unsigned NumContainedTys;

  /// For a named struct that actually has a name, this is a pointer to the
  /// symbol table entry for the struct. This is null if the type is an
  /// literal struct or if it is a identified type that has an empty name.
public:
  StructType(std::vector<IRTyPtr> members, std::string Name, bool isliteral);
  /// Create identified struct.
  /*static IRStructTyPtr Create(std::string Name);
                  static IRStructTyPtr Create(MosesIRContext &Ctx,
     std::vector<IRTyPtr> Elements, std::string Name);*/
  static IRStructTyPtr Create(MosesIRContext &Ctx, ASTTyPtr type);

  /// Create literal struct type.
  static IRStructTyPtr get(std::vector<IRTyPtr> Elements);
  /// Create literal struct type.
  static IRStructTyPtr get(MosesIRContext &Ctx, ASTTyPtr type);

  virtual unsigned getSize() const override;

  bool isLiteral() const { return Literal; }

  /// Return the name for this struct type if it has an identity.
  /// This may return an empty string for an unnamed struct type. Do not call
  /// this on an literal type.
  std::string getName() const;

  /// Change the name of this type to the specified name.
  void setName(std::string Name);

  bool isLayoutIdentical(IRStructTyPtr Other) const;
  unsigned getNumElements() const { return NumContainedTys; }
  std::vector<std::shared_ptr<Type>> getContainedTys() const {
    return ContainedTys;
  }
  IRTyPtr operator[](unsigned index) const {
    assert(index < NumContainedTys && "Index out of range!");
    return ContainedTys[index];
  }

  static bool classof(IRTyPtr T) { return T->getTypeID() == TypeID::StructTy; }

  /// \brief Print the StructType Info.
  void Print(std::ostringstream &out) override;
  void PrintCompleteInfo(std::ostringstream &out);
};

/// PointerType - Class to represent pointers
class PointerType : public IR::Type {
  IRTyPtr ElementTy;

public:
  PointerType(IRTyPtr ElementTy);
  static IRPtTyPtr get(IRTyPtr ElementTy);
  IRTyPtr getElementTy() const { return ElementTy; };
  static bool classof(IRPtTyPtr) { return true; }
  static bool classof(IRTyPtr Ty) { return Ty->isPointerTy(); }
  /// \brief Print the PointerType.
  void Print(std::ostringstream &out) override;
};
} // namespace IR
