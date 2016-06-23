//===----------------------------ConstantFolder.h-------------------------===//
//
// This file defines the ConstantFolder class, a helper for IRBuilder.
// It provides IRBuilder with a set of methods for creating constants
// with minimul folding. For general constant creation and folding,
// use ConstantExpr and the routines in llvm/Analysis/ConstantFolding.h
// 
// Note: 在LLVM中，提供了一个简单的ConstantFolder，该ConstantFolder只是简单
// 的对constant进行折叠。AST上的constant-folding是为了在代码生成时，提供一些
// 简单的帮助，例如: if(condition) {} else {}.
//
//===---------------------------------------------------------------------===//
#ifndef MOSES_IR_CONSTANT_FOLDER_H
#define MOSES_IR_CONSTANT_FOLDER_H
#include "../IR/Instruction.h"
#include "../IR/ConstantAndGlobal.h"

namespace compiler
{
	namespace IR
	{
		/// ConstantFolder - Create constants with minimum, target independent, folding.
		class ConstantFolder
		{
		public:
			//===--------------------------------------------------------------===//
			// Binary Operators - Arithmetic
			//===--------------------------------------------------------------===//
			static ConstantIntPtr CreateArithmetic(BinaryOperator::Opcode Op, ConstantIntPtr LHS, 
				ConstantIntPtr  RHS)
			{
				switch (Op)
				{
				case compiler::IR::Instruction::Opcode::Add:
					return std::make_shared<ConstantInt>(LHS->getVal() + RHS->getVal());
				case compiler::IR::Instruction::Opcode::Sub:
					return std::make_shared<ConstantInt>(LHS->getVal() - RHS->getVal());
				case compiler::IR::Instruction::Opcode::Mul:
					return std::make_shared<ConstantInt>(LHS->getVal() * RHS->getVal());
				case compiler::IR::Instruction::Opcode::Div:
					return std::make_shared<ConstantInt>(LHS->getVal() / RHS->getVal());
				case compiler::IR::Instruction::Opcode::Rem:
					return std::make_shared<ConstantInt>(LHS->getVal() % RHS->getVal());
				case compiler::IR::Instruction::Opcode::Shl:
					return std::make_shared<ConstantInt>(LHS->getVal() << RHS->getVal());
				case compiler::IR::Instruction::Opcode::Shr:
					return std::make_shared<ConstantInt>(LHS->getVal() >> RHS->getVal());
				default:
					return nullptr;
				}
			}
			
			//===--------------------------------------------------------------===//
			// Binary Operators - boolean
			//===--------------------------------------------------------------===//
			static ConstantBoolPtr CreateBoolean(BinaryOperator::Opcode Op, ConstantBoolPtr LHS,
				ConstantBoolPtr RHS)
			{
				switch (Op)
				{
				case compiler::IR::Instruction::Opcode::And:
					return std::make_shared<ConstantBool>(LHS->getVal() && RHS->getVal());
				case compiler::IR::Instruction::Opcode::Or:
					return std::make_shared<ConstantBool>(LHS->getVal() || RHS->getVal());
				default:
					return nullptr;
				}
			}
			
			static ConstantBoolPtr CreateCmp(CmpInst::Predicate P, ConstantIntPtr LHS,
				ConstantIntPtr RHS)
			{
				switch (P)
				{
				case compiler::IR::CmpInst::CMP_EQ:
					return std::make_shared<ConstantBool>(LHS->getVal() == RHS->getVal());
				case compiler::IR::CmpInst::CMP_NE:
					return std::make_shared<ConstantBool>(LHS->getVal() != RHS->getVal());
				case compiler::IR::CmpInst::CMP_GT:
					return std::make_shared<ConstantBool>(LHS->getVal() > RHS->getVal());
				case compiler::IR::CmpInst::CMP_GE:
					return std::make_shared<ConstantBool>(LHS->getVal() >= RHS->getVal());
				case compiler::IR::CmpInst::CMP_LT:
					return std::make_shared<ConstantBool>(LHS->getVal() < RHS->getVal());
				case compiler::IR::CmpInst::CMP_LE:
					return std::make_shared<ConstantBool>(LHS->getVal() <= RHS->getVal());
				default:
					return nullptr;
				}
			}

			static ConstantBoolPtr CreateCmp(CmpInst::Predicate P, ConstantBoolPtr LHS,
				ConstantBoolPtr RHS)
			{
				switch (P)
				{
				case compiler::IR::CmpInst::CMP_EQ:
					return std::make_shared<ConstantBool>(LHS->getVal() == RHS->getVal());
				case compiler::IR::CmpInst::CMP_NE:
					return std::make_shared<ConstantBool>(LHS->getVal() != RHS->getVal());
				default:
					return nullptr;
				}
			}

			//===----------------------------------------------------------===//
			// Unary Oeprators
			//===----------------------------------------------------------===//
			static ConstantIntPtr CreateNeg(ConstantIntPtr C)
			{
				return std::make_shared<ConstantInt>(0 - C->getVal());
			}

			static ConstantBoolPtr CreateNot(ConstantBoolPtr C)
			{
				return std::make_shared<ConstantBool>(!C->getVal());
			}
		};
	}
}

#endif