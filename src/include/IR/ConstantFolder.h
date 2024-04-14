//===----------------------------ConstantFolder.h-------------------------===//
//
// This file defines the ConstantFolder class, a helper for IRBuilder.
// It provides IRBuilder with a set of methods for creating constants
// with minimul folding. For general constant creation and folding,
// use ConstantExpr and the routines in llvm/Analysis/ConstantFolding.h
//
//===---------------------------------------------------------------------===//
#pragma once
#include "ConstantAndGlobal.h"
#include "Instruction.h"

namespace IR {
/// ConstantFolder - Create constants with minimum, target independent, folding.
class ConstantFolder {
public:
  //===--------------------------------------------------------------===//
  // Binary Operators - Arithmetic
  //===--------------------------------------------------------------===//
  static std::shared_ptr<ConstantInt> CreateArithmetic(MosesIRContext &Ctx,
                                         BinaryOperator::Opcode Op,
                                         std::shared_ptr<ConstantInt> LHS,
                                         std::shared_ptr<ConstantInt> RHS) {
    switch (Op) {
    case IR::Instruction::Opcode::Add:
      return std::make_shared<ConstantInt>(Ctx, LHS->getVal() + RHS->getVal());
    case IR::Instruction::Opcode::Sub:
      return std::make_shared<ConstantInt>(Ctx, LHS->getVal() - RHS->getVal());
    case IR::Instruction::Opcode::Mul:
      return std::make_shared<ConstantInt>(Ctx, LHS->getVal() * RHS->getVal());
    case IR::Instruction::Opcode::Div:
      return std::make_shared<ConstantInt>(Ctx, LHS->getVal() / RHS->getVal());
    case IR::Instruction::Opcode::Rem:
      return std::make_shared<ConstantInt>(Ctx, LHS->getVal() % RHS->getVal());
    case IR::Instruction::Opcode::Shl:
      return std::make_shared<ConstantInt>(Ctx, LHS->getVal() << RHS->getVal());
    case IR::Instruction::Opcode::Shr:
      return std::make_shared<ConstantInt>(Ctx, LHS->getVal() >> RHS->getVal());
    default:
      return nullptr;
    }
  }

  //===--------------------------------------------------------------===//
  // Binary Operators - boolean
  //===--------------------------------------------------------------===//
  static std::shared_ptr<ConstantBool> CreateBoolean(MosesIRContext &Ctx,
                                       BinaryOperator::Opcode Op,
                                       std::shared_ptr<ConstantBool> LHS,
                                       std::shared_ptr<ConstantBool> RHS) {
    switch (Op) {
    case IR::Instruction::Opcode::And:
      return std::make_shared<ConstantBool>(Ctx,
                                            LHS->getVal() && RHS->getVal());
    case IR::Instruction::Opcode::Or:
      return std::make_shared<ConstantBool>(Ctx,
                                            LHS->getVal() || RHS->getVal());
    default:
      return nullptr;
    }
  }

  static std::shared_ptr<ConstantBool> CreateCmp(MosesIRContext &Ctx, CmpInst::Predicate P,
                                   std::shared_ptr<ConstantInt> LHS, std::shared_ptr<ConstantInt> RHS) {
    switch (P) {
    case IR::CmpInst::CMP_EQ:
      return std::make_shared<ConstantBool>(Ctx,
                                            LHS->getVal() == RHS->getVal());
    case IR::CmpInst::CMP_NE:
      return std::make_shared<ConstantBool>(Ctx,
                                            LHS->getVal() != RHS->getVal());
    case IR::CmpInst::CMP_GT:
      return std::make_shared<ConstantBool>(Ctx, LHS->getVal() > RHS->getVal());
    case IR::CmpInst::CMP_GE:
      return std::make_shared<ConstantBool>(Ctx,
                                            LHS->getVal() >= RHS->getVal());
    case IR::CmpInst::CMP_LT:
      return std::make_shared<ConstantBool>(Ctx, LHS->getVal() < RHS->getVal());
    case IR::CmpInst::CMP_LE:
      return std::make_shared<ConstantBool>(Ctx,
                                            LHS->getVal() <= RHS->getVal());
    default:
      return nullptr;
    }
  }

  static std::shared_ptr<ConstantBool> CreateCmp(MosesIRContext &Ctx, CmpInst::Predicate P,
                                   std::shared_ptr<ConstantBool> LHS, std::shared_ptr<ConstantBool> RHS) {
    switch (P) {
    case IR::CmpInst::CMP_EQ:
      return std::make_shared<ConstantBool>(Ctx,
                                            LHS->getVal() == RHS->getVal());
    case IR::CmpInst::CMP_NE:
      return std::make_shared<ConstantBool>(Ctx,
                                            LHS->getVal() != RHS->getVal());
    default:
      return nullptr;
    }
  }

  //===----------------------------------------------------------===//
  // Unary Oeprators
  //===----------------------------------------------------------===//
  static std::shared_ptr<ConstantInt> CreateNeg(MosesIRContext &Ctx, std::shared_ptr<ConstantInt> C) {
    return std::make_shared<ConstantInt>(Ctx, 0 - C->getVal());
  }

  static std::shared_ptr<ConstantBool> CreateNot(MosesIRContext &Ctx, std::shared_ptr<ConstantBool> C) {
    return std::make_shared<ConstantBool>(Ctx, !C->getVal());
  }
};
} // namespace IR
