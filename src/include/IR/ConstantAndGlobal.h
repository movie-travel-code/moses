//===----------------------------ConstantAndGlobal.hh---------------------===//
//
// This file contains the declaration of the Constant class.
//
// GlobalValue is a common base class of all globally definable objects. As
// such, it is subclassed by GlobalVariable and by Function. This is used
// because you can do certain things with these global objects that you can't
// do to anything else. For example, use the address of one as a constant.
//===---------------------------------------------------------------------===//
#ifndef MOSES_IR_CONSTANT_AND_GLOBAL_H
#define MOSES_IR_CONSTANT_AND_GLOBAL_H
#include "User.h"

namespace IR {
class ArrayType;
class StructType;
class PointerType;
class Constant;
class MosesIRContext;

using ConstantPtr = std::shared_ptr<Constant>;

class Constant : public User {
  void operator=(const Constant &) = delete;
  Constant(const Constant &) = delete;

protected:
  Constant(TyPtr Ty) : User(Ty, Value::ValueTy::ConstantVal) {}
  ~Constant() {}

public:
  // setName - Specialize setName to handle symbol table majik...(��Ϸ)
  // virtual void setName(const std::string &name, SymbolTable* ST = 0);

  /// isConstantExpr - Return true if this is a ConstantExpr
  virtual bool isConstantExpr() const { return false; }
  virtual void destoryConstant() {}
  bool isOneValue() const;

  /// Methods for support type inquiry through isa, cast, and dyn_cast:
  static bool classof(const Constant *) { return true; }
  static bool classof(const Value *V) {
    return V->getValueType() == Value::ValueTy::ConstantVal;
  }
};

class PointerType;
class GlobalValue : public User {
  GlobalValue(const GlobalValue &) = delete;

protected:
  TyPtr ValTy;
  GlobalValue(TyPtr Ty, ValueTy vty, const std::string &name = "");

public:
  TyPtr getType() const { return Ty; }
  TyPtr getValType() const { return ValTy; }
  static bool classof(const GlobalValue *T) { return true; }
  static bool classof(const Value *V) {
    return V->getValueType() == Value::ValueTy::FunctionVal ||
           V->getValueType() == Value::ValueTy::GlobalVariableVal;
  }
};

//===------------------------------------------------------------------===//
// This is the shared class of boolean and integer constants. This class
// represents both boolean and integral constants.
// \brief Class for constant integers.
class ConstantIntegral : public Constant {
  ConstantIntegral(const ConstantIntegral &) = delete;

protected:
  explicit ConstantIntegral(TyPtr ty) : Constant(ty) {}

public:
  /// Methods for support type inquiry through isa, cast, and dyn_cast:
  static bool classof(const ConstantIntegral *) { return true; }
  static bool classof(const Constant *CPV); // defined in Constants.cpp
  // To Do:
  static bool classof(const Value *V) { return true; }
};

//===----------------------------------------------------------------===//
// ConsrantBool - Boolean Values
class ConstantBool final : public ConstantIntegral {
private:
  bool Val;

public:
  ConstantBool(MosesIRContext &Ctx, bool val);
  static ConstantBool *True, *False;

  /// getTrue() - static factory methods - Return objects of the specified
  /// value.
  static ConstantBoolPtr getTrue(MosesIRContext &Ctx) {
    return std::make_shared<ConstantBool>(Ctx, true);
  }
  /// getFalse() - static factory methods - Return objects of the specified
  /// value.
  static ConstantBoolPtr getFalse(MosesIRContext &Ctx) {
    return std::make_shared<ConstantBool>(Ctx, false);
  }
  /// inverted - Return the opposite value of the current value.
  ConstantBool *inverted() const { return (this == True) ? False : True; }
  /// getVal - return the boolean value of this constant.
  bool getVal() const { return Val; }

  /// Methods for support type inquiry through isa, cast, and dyn_cast:
  static bool classof(const ConstantBool *) { return true; }
  static bool classof(const Constant *CPV) {
    return (CPV == True) | (CPV == False);
  }

  /// \brief Print the ConstantBool.
  void Print(std::ostringstream &out) override;
};

// ��moses�У���ʱֻ��int��bool���ͣ�int���Ϳ���ʹ��i32��ʾ��
class ConstantInt final : public ConstantIntegral {
  int Val;
  ConstantInt(const ConstantInt &) = delete;

public:
  ConstantInt(MosesIRContext &Ctx, int val);

  /// \brief Get a ConstantInt for a specific value.
  static ConstantIntPtr get(MosesIRContext &Ctx, int value) {
    auto CI = std::make_shared<ConstantInt>(Ctx, value);
    CI->setName(std::to_string(value));
    return CI;
  }

  bool equalsInt(int v) const { return Val == v; }
  int getVal() const { return Val; }
  // Shit code.
  static ConstantIntPtr getZeroValueForNegative(MosesIRContext &Ctx) {
    return std::make_shared<ConstantInt>(Ctx, 0);
  }

  static bool classof(ValPtr V) {
    return V->getValueType() == Value::ValueTy::ConstantVal;
  }

  /// \brief Print the ConstantInt.
  void Print(std::ostringstream &out) override;
};

//===----------------------------------------------------------------===//
// ConstantSturct - Constant Struct Declarations
class ConstantStruct : public Constant {
  ConstantStruct(const ConstantStruct &) = delete;

protected:
  ConstantStruct(const StructType *T, const std::vector<Constant *> &Val);

public:
  /// get() - Static factory methods - Return objects of the specified value
  static ConstantStruct *get(const StructType *T,
                             const std::vector<Constant *> &V);

  /// getType() specialization - Reduce amount of casting...
  const StructType *getType() const { return nullptr; }

  // Methods for support type inquiry through isa, cast, and dyn_cast:
  static bool classof(std::shared_ptr<ConstantStruct>) { return true; }

  /// getValues - Return a vector of the component constants that make up
  /// this structure.
  // const std::vector<Use> &getValue() const { return Operands; }

  /// \brief Print the ConstantStruct.
  void Print(std::ostringstream &out) override;
};
} // namespace IR

#endif