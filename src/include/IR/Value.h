//===-------------------------------Value.h-------------------------------===//
//
// This file defines the very important Value class. This is subclassed by a
// bunch of other important classes, like Instruction, Function, Type, etc...
//
// moses IR�ο���LLVM IR��ע�Ͳ���ժ��LLVM Value.h (�²ۣ�ΪʲôҪ��Value�������)
//
//===---------------------------------------------------------------------===//
#ifndef MOSES_IR_VALUE
#define MOSES_IR_VALUE
#include "IRType.h"
#include <cassert>
#include <iostream>
#include <list>
#include <memory>
#include <sstream>
#include <string>

//===---------------------------------------------------------------------===//
//						Value Class
//===---------------------------------------------------------------------===//

/// Value - The base class of all values computed by a program that may be used
/// as operands to other values. Value is the super class of other important
/// classes such as Instruction and Function. All Values have a Type. Type is
/// not a subclass of Value.
///
/// Every value has a "use list" that keeps track of which other Values are
/// using this value.
/// ------------------------------nonsense for coding------------------------
///	Value��moses
///IR�еĻ��࣬�������﷨����statement��valueֻ����һЩ�򵥵Ĳ�����
/// moses IR��һ������IR��������CFG��Ϣ��ͨ��Block����֯����������SSA-based�ģ�
/// �����ں�def-use��Ϣ��Ϊ�˸�Ч�Ĳ��ң�ʹ��ɢ�б���Ϊ���ű�Ļ����ṹ����Ϊ��
///	���㣬���ű���ʱ���Ϊvector����
/// -------------------------------------------------------------------------
namespace compiler {
namespace IR {
class Type;
class Constant;
class Argument;
class Instruction;
class BasicBlock;
class GlobalValue;
class CmpInst;
class AllocaInst;
class GetElementPtrInst;
class CallInst;
class Function;
class Intrinsic;
class FunctionType;
class ExtractValueInst;
class PHINode;
class ReturnInst;
class GlobalVariable;
class ValueSymbolTable;
class ValueSymbolTable;
class BinaryOperator;
class Argument;
class TerminatorInst;
class Constant;
class ConstantBool;
class ConstantInt;
class BranchInst;
class UnaryOperator;
class LoadInst;
class StoreInst;
class GetElementPtrInst;
class StructType;
class Use;
class Value;
class User;

typedef std::shared_ptr<Type> TyPtr;
typedef std::shared_ptr<Value> ValPtr;
typedef std::shared_ptr<Use> UsePtr;
typedef std::shared_ptr<Instruction> InstPtr;
typedef std::shared_ptr<BasicBlock> BBPtr;
typedef std::shared_ptr<BinaryOperator> BOInstPtr;
typedef std::shared_ptr<CmpInst> CmpInstPtr;
typedef std::shared_ptr<GetElementPtrInst> GEPInstPtr;
typedef std::shared_ptr<CallInst> CallInstPtr;
typedef std::shared_ptr<ExtractValueInst> EVInstPtr;
typedef std::shared_ptr<PHINode> PHINodePtr;
typedef std::shared_ptr<ReturnInst> RetInstPtr;
typedef std::shared_ptr<Function> FuncPtr;
typedef std::shared_ptr<ValueSymbolTable> SymTabPtr;
typedef std::shared_ptr<User> UserPtr;
typedef std::shared_ptr<Argument> ArgPtr;
typedef std::shared_ptr<TerminatorInst> TermiPtr;
typedef std::shared_ptr<ReturnInst> ReturnInstPtr;
typedef std::shared_ptr<ConstantBool> ConstantBoolPtr;
typedef std::shared_ptr<ConstantInt> ConstantIntPtr;
typedef std::shared_ptr<BranchInst> BrInstPtr;
typedef std::shared_ptr<UnaryOperator> UOInstPtr;
typedef std::shared_ptr<AllocaInst> AllocaInstPtr;
typedef std::shared_ptr<LoadInst> LoadInstPtr;
typedef std::shared_ptr<StoreInst> StoreInstPtr;
typedef std::shared_ptr<GetElementPtrInst> GEPInstPtr;
typedef std::shared_ptr<FunctionType> FuncTypePtr;
typedef std::shared_ptr<StructType> StructTypePtr;
typedef std::list<InstPtr>::iterator Iterator;
typedef std::shared_ptr<Intrinsic> IntrinsicPtr;
class Value {
public:
  // ------------------nonsense for coding------------------------
  // LLVM IR is strongly typed, every instruction has a specific
  // type it expects as each operand. For example, store requires
  // that the address's type will always be a pointer to the type
  // of the value being stored.
  enum class ValueTy {
    TypeVal,          // This is an instance of Type
    ConstantVal,      // This is an instance of Constant
    ArgumentVal,      // This is an instance of Argument
    InstructionVal,   // This is an instance of Instruction
    BasicBlockVal,    // This is an instance of BasicBlock
    FunctionVal,      // This is an instance of Function
    GlobalVariableVal // This is an instance of VlobalVariable
  };

protected:
  // a "use list" that keep track of which other Values are using this value.
  std::list<Use *> Uses;
  std::string Name;
  ValueTy VTy;
  TyPtr Ty;
  void operator=(const Value &) = delete; // Do not implement
  Value(const Value &) = delete;          // Do not implement

public:
  // 'Type' stands for Value's type, like integer, void or FunctionType.
  // 'ValueTy' stands for category, like Instruction, ConstantInt or Function.
  Value(std::shared_ptr<Type> Ty, ValueTy vty, std::string name = "");
  virtual ~Value();

  // All values are typed, get the type of this value.
  TyPtr getType() const { return Ty; }
  // hasOneUse - Return true if there is exactly one user of this value.
  bool hasOneUse() const;
  bool hasName() const { return Name != ""; }
  const std::string &getName() const { return Name; }
  virtual void setName(const std::string &name) { Name = name; }

  /// getValueType - Return the immediate subclass of this Value.
  ValueTy getValueType() const { return VTy; }

  /// replaceAllUsesWith - Go through the use list for this definition and make
  /// each use point to "V" instead of "this". After this completes, this's
  /// use list is guaranteed to be empty.
  void replaceAllUsesWith(ValPtr NewV);

  //---------------------------------------------------------------------
  // Methods for handling the vector of uses of this Value.
  const Value *use_begin() const;
  unsigned use_size() const { return Uses.size(); }
  bool use_empty() const { return Uses.empty(); }
  const std::list<Use *> &getUses() const { return Uses; }

  /// addUse/killUse - These two methods should only be used by the Use class.
  /// This means that every time we create a new Use object, the use object
  /// will be linked to the Value's Uses(std::list);
  void addUse(Use &U);
  void killUse(Use &U);

  /// \brief Print the Value info.
  virtual void Print(std::ostringstream &out) = 0;
};

/// -----------------------------LLVM annotation-----------------------------
/// \brief A Use represents the edge between a Value definition and its users.
///
/// This is notionally a two-dimensional linked list. It supports traversing
/// all of the uses for a particular value definition. It also supporrts jumping
/// directly to the used value when we arrive from the User's operands, and
/// jumping directly to the User when we arrive from the Value's uses.
/// ------------------------------LLVM annotation-----------------------------
/// Use Objects linked up according to the 'Val', the Use objects with the same
/// Value must be on the same list.
/// e.g.  Val1  %retval = alloca i32  UseList >> Use1---->Use2-----> ...    must
/// have the same Vals
///             ...
///       User1 store i32 10, i32* %retval =======> Use1 [ Val1 | User1]
///             ...
///       User2 %tmp = load i32* %retval   =======> Use2 [ Val1 | User2]
/// BTW, User can get the use objects through the 'Operands <vector>'.
class Use {
private:
  User *U;
  ValPtr Val;

public:
  Use() : Val(nullptr), U(nullptr) {}
  Use(ValPtr Val, User *U);
  Use(const Use &u);
  ~Use();

  ValPtr get() const { return Val; }
  const Value *getUser() const { return reinterpret_cast<Value *>(U); }
  void set(ValPtr Val);

  ValPtr operator=(ValPtr RHS);
  const Use &operator=(Use RHS);
  bool operator==(const Use &use);
};
} // namespace IR
} // namespace compiler
#endif