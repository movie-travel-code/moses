//===----------------------------ConstantFolder.h-------------------------===//
//
// This file defines the ConstantFolder class, a helper for IRBuilder.
// It provides IRBuilder with a set of methods for creating constants
// with minimul folding. For general constant creation and folding,
// use ConstantExpr and the routines in llvm/Analysis/ConstantFolding.h
//
// Note:
// ��LLVM�У��ṩ��һ���򵥵�ConstantFolder����ConstantFolderֻ�Ǽ�
// �Ķ�constant�����۵���AST�ϵ�constant-folding��Ϊ���ڴ�������ʱ���ṩһЩ
// �򵥵İ���������: if(condition) {} else {}.
//
//===---------------------------------------------------------------------===//
#ifndef MOSES_IR_CONSTANT_FOLDER_H
#define MOSES_IR_CONSTANT_FOLDER_H
#include "ConstantAndGlobal.h"
#include "Instruction.h"


namespace compiler {
namespace IR {
/// ConstantFolder - Create constants with minimum, target independent, folding.
class ConstantFolder {
public:
  //===--------------------------------------------------------------===//
  // Binary Operators - Arithmetic
  //===--------------------------------------------------------------===//
  static ConstantIntPtr CreateArithmetic(MosesIRContext &Ctx,
                                         BinaryOperator::Opcode Op,
                                         ConstantIntPtr LHS,
                                         ConstantIntPtr RHS) {
    switch (Op) {
    case compiler::IR::Instruction::Opcode::Add:
      return std::make_shared<ConstantInt>(Ctx, LHS->getVal() + RHS->getVal());
    case compiler::IR::Instruction::Opcode::Sub:
      return std::make_shared<ConstantInt>(Ctx, LHS->getVal() - RHS->getVal());
    case compiler::IR::Instruction::Opcode::Mul:
      return std::make_shared<ConstantInt>(Ctx, LHS->getVal() * RHS->getVal());
    case compiler::IR::Instruction::Opcode::Div:
      return std::make_shared<ConstantInt>(Ctx, LHS->getVal() / RHS->getVal());
    case compiler::IR::Instruction::Opcode::Rem:
      return std::make_shared<ConstantInt>(Ctx, LHS->getVal() % RHS->getVal());
    case compiler::IR::Instruction::Opcode::Shl:
      return std::make_shared<ConstantInt>(Ctx, LHS->getVal() << RHS->getVal());
    case compiler::IR::Instruction::Opcode::Shr:
      return std::make_shared<ConstantInt>(Ctx, LHS->getVal() >> RHS->getVal());
    default:
      return nullptr;
    }
  }

  //===--------------------------------------------------------------===//
  // Binary Operators - boolean
  //===--------------------------------------------------------------===//
  static ConstantBoolPtr CreateBoolean(MosesIRContext &Ctx,
                                       BinaryOperator::Opcode Op,
                                       ConstantBoolPtr LHS,
                                       ConstantBoolPtr RHS) {
    switch (Op) {
    case compiler::IR::Instruction::Opcode::And:
      return std::make_shared<ConstantBool>(Ctx,
                                            LHS->getVal() && RHS->getVal());
    case compiler::IR::Instruction::Opcode::Or:
      return std::make_shared<ConstantBool>(Ctx,
                                            LHS->getVal() || RHS->getVal());
    default:
      return nullptr;
    }
  }

  static ConstantBoolPtr CreateCmp(MosesIRContext &Ctx, CmpInst::Predicate P,
                                   ConstantIntPtr LHS, ConstantIntPtr RHS) {
    switch (P) {
    case compiler::IR::CmpInst::CMP_EQ:
      return std::make_shared<ConstantBool>(Ctx,
                                            LHS->getVal() == RHS->getVal());
    case compiler::IR::CmpInst::CMP_NE:
      return std::make_shared<ConstantBool>(Ctx,
                                            LHS->getVal() != RHS->getVal());
    case compiler::IR::CmpInst::CMP_GT:
      return std::make_shared<ConstantBool>(Ctx, LHS->getVal() > RHS->getVal());
    case compiler::IR::CmpInst::CMP_GE:
      return std::make_shared<ConstantBool>(Ctx,
                                            LHS->getVal() >= RHS->getVal());
    case compiler::IR::CmpInst::CMP_LT:
      return std::make_shared<ConstantBool>(Ctx, LHS->getVal() < RHS->getVal());
    case compiler::IR::CmpInst::CMP_LE:
      return std::make_shared<ConstantBool>(Ctx,
                                            LHS->getVal() <= RHS->getVal());
    default:
      return nullptr;
    }
  }

  static ConstantBoolPtr CreateCmp(MosesIRContext &Ctx, CmpInst::Predicate P,
                                   ConstantBoolPtr LHS, ConstantBoolPtr RHS) {
    switch (P) {
    case compiler::IR::CmpInst::CMP_EQ:
      return std::make_shared<ConstantBool>(Ctx,
                                            LHS->getVal() == RHS->getVal());
    case compiler::IR::CmpInst::CMP_NE:
      return std::make_shared<ConstantBool>(Ctx,
                                            LHS->getVal() != RHS->getVal());
    default:
      return nullptr;
    }
  }

  //===----------------------------------------------------------===//
  // Unary Oeprators
  //===----------------------------------------------------------===//
  static ConstantIntPtr CreateNeg(MosesIRContext &Ctx, ConstantIntPtr C) {
    return std::make_shared<ConstantInt>(Ctx, 0 - C->getVal());
  }

  static ConstantBoolPtr CreateNot(MosesIRContext &Ctx, ConstantBoolPtr C) {
    return std::make_shared<ConstantBool>(Ctx, !C->getVal());
  }
};
} // namespace IR
} // namespace compiler

#endif